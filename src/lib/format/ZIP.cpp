#include <vpkedit/format/ZIP.h>

#include <cstring>
#include <filesystem>

#include <mz.h>
#include <mz_strm.h>
#include <mz_strm_os.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

const std::string ZIP::TEMP_ZIP_PATH = (std::filesystem::temp_directory_path() / "tmp.zip").string();

ZIP::ZIP(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFile(fullFilePath_, options_) {
	this->type = PackFileType::ZIP;
}

ZIP::~ZIP() {
	this->closeZIP();
}

std::unique_ptr<PackFile> ZIP::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* zip = new ZIP{path, options};
	auto packFile = std::unique_ptr<PackFile>(zip);

	if (!zip->openZIP(zip->fullFilePath)) {
		return nullptr;
	}

	for (auto code = mz_zip_goto_first_entry(zip->zipHandle); code == MZ_OK; code = mz_zip_goto_next_entry(zip->zipHandle)) {
		mz_zip_file* fileInfo = nullptr;
		if (mz_zip_entry_get_info(zip->zipHandle, &fileInfo)) {
			return nullptr;
		}
		if (mz_zip_entry_is_dir(zip->zipHandle) == MZ_OK) {
			continue;
		}

		Entry entry = createNewEntry();
		entry.path = fileInfo->filename;
		::normalizeSlashes(entry.path);
		if (!options.allowUppercaseLettersInFilenames) {
			::toLowerCase(entry.path);
		}

		entry.flags = fileInfo->compression_method;
		entry.length = fileInfo->uncompressed_size;
		entry.compressedLength = fileInfo->compressed_size;
		entry.crc32 = fileInfo->crc;

		auto parentDir = std::filesystem::path(entry.path).parent_path().string();
		::normalizeSlashes(parentDir);
		if (!options.allowUppercaseLettersInFilenames) {
			::toLowerCase(parentDir);
		}
		if (!zip->entries.contains(parentDir)) {
			zip->entries[parentDir] = {};
		}
		zip->entries[parentDir].push_back(entry);

		if (callback) {
			callback(parentDir, entry);
		}
	}

	return packFile;
}

std::optional<std::vector<std::byte>> ZIP::readEntry(const Entry& entry) const {
	if (entry.unbaked) {
		// Get the stored data
		for (const auto& [unbakedEntryDir, unbakedEntryList] : this->unbakedEntries) {
			for (const Entry& unbakedEntry : unbakedEntryList) {
				if (unbakedEntry.path == entry.path) {
					std::vector<std::byte> unbakedData;
					if (isEntryUnbakedUsingByteBuffer(unbakedEntry)) {
						unbakedData = std::get<std::vector<std::byte>>(getEntryUnbakedData(unbakedEntry));
					} else {
						unbakedData = ::readFileData(std::get<std::string>(getEntryUnbakedData(unbakedEntry)));
					}
					return unbakedData;
				}
			}
		}
		return std::nullopt;
	}
	// It's baked into the file on disk
	if (!this->streamOpen || !this->zipOpen) {
		return std::nullopt;
	}
	if (mz_zip_locate_entry(this->zipHandle, entry.path.c_str(), !this->options.allowUppercaseLettersInFilenames) != MZ_OK) {
		return std::nullopt;
	}
	if (mz_zip_entry_read_open(this->zipHandle, 0, nullptr) != MZ_OK) {
		return std::nullopt;
	}
	std::vector<std::byte> out;
	out.resize(entry.length);
	mz_zip_entry_read(this->zipHandle, out.data(), static_cast<int>(entry.length));
	mz_zip_entry_close(this->zipHandle);
	return out;
}

Entry& ZIP::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	auto filename = filename_;
	if (!this->options.allowUppercaseLettersInFilenames) {
		::toLowerCase(filename);
	}
	auto [dir, name] = ::splitFilenameAndParentDir(filename);

	entry.path = filename;
	entry.flags = options_.zip_compressionMethod;
	entry.length = buffer.size();
	entry.compressedLength = 0;
	entry.crc32 = ::computeCRC32(buffer);

	if (!this->unbakedEntries.contains(dir)) {
		this->unbakedEntries[dir] = {};
	}
	this->unbakedEntries.at(dir).push_back(entry);
	return this->unbakedEntries.at(dir).back();
}

bool ZIP::bake(const std::string& outputDir_, const Callback& callback) {
	// Get the proper file output folder
	std::string outputDir = this->getBakeOutputDir(outputDir_);
	std::string outputPath = outputDir + '/' + this->getFilename();

	// Use temp folder so we can read from the current ZIP
	if (!this->bakeTempZip(ZIP::TEMP_ZIP_PATH, callback)) {
		return false;
	}
	this->mergeUnbakedEntries();

	// Close our ZIP and reopen it
	this->closeZIP();
	std::filesystem::rename(ZIP::TEMP_ZIP_PATH, outputPath);
	if (!this->openZIP(outputPath)) {
		return false;
	}
	PackFile::setFullFilePath(outputDir);
	return true;
}

std::vector<Attribute> ZIP::getSupportedEntryAttributes() const {
	using enum Attribute;
	return {LENGTH, CRC32};
}

bool ZIP::bakeTempZip(const std::string& writeZipPath, const Callback& callback) {
	void* writeStreamHandle = mz_stream_os_create();
	if (mz_stream_os_open(writeStreamHandle, writeZipPath.c_str(), MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE)) {
		return false;
	}

	void* writeZipHandle = mz_zip_writer_create();
	if (mz_zip_writer_open(writeZipHandle, writeStreamHandle, 0)) {
		return false;
	}

	for (const auto& [entryDir, entries] : this->getBakedEntries()) {
		for (const Entry& entry : entries) {
			auto binData = this->readEntry(entry);
			if (!binData) {
				continue;
			}

			mz_zip_file fileInfo;
			std::memset(&fileInfo, 0, sizeof(mz_zip_entry));
			fileInfo.flag = MZ_ZIP_FLAG_DATA_DESCRIPTOR;
			fileInfo.filename = entry.path.c_str();
			fileInfo.filename_size = entry.path.length();
			fileInfo.uncompressed_size = static_cast<std::int64_t>(entry.length);
			fileInfo.compressed_size = static_cast<std::int64_t>(entry.compressedLength);
			fileInfo.crc = entry.crc32;
			fileInfo.compression_method = entry.flags;
			if (mz_zip_writer_add_buffer(writeZipHandle, binData->data(), static_cast<int>(binData->size()), &fileInfo)) {
				return false;
			}

			if (callback) {
				callback(entry.getParentPath(), entry);
			}
		}
	}
	for (const auto& [entryDir, entries] : this->getUnbakedEntries()) {
		for (const Entry& entry : entries) {
			auto binData = this->readEntry(entry);
			if (!binData) {
				continue;
			}

			mz_zip_entry fileInfo;
			std::memset(&fileInfo, 0, sizeof(mz_zip_entry));
			fileInfo.filename = entry.path.c_str();
			fileInfo.filename_size = entry.path.length();
			fileInfo.uncompressed_size = static_cast<std::int64_t>(entry.length);
			fileInfo.compressed_size = static_cast<std::int64_t>(entry.compressedLength);
			fileInfo.crc = entry.crc32;
			fileInfo.compression_method = entry.flags;
			if (mz_zip_writer_add_buffer(writeZipHandle, binData->data(), static_cast<int>(binData->size()), &fileInfo)) {
				return false;
			}

			if (callback) {
				callback(entry.getParentPath(), entry);
			}
		}
	}

	if (mz_zip_writer_close(writeZipHandle)) {
		return false;
	}
	mz_zip_writer_delete(&writeZipHandle);

	if (mz_stream_os_close(writeStreamHandle)) {
		return false;
	}
	mz_stream_os_delete(&writeStreamHandle);

	return true;
}

bool ZIP::openZIP(std::string_view path) {
	this->streamHandle = mz_stream_os_create();
	if (mz_stream_os_open(this->streamHandle, path.data(), MZ_OPEN_MODE_READ) != MZ_OK) {
		return false;
	}
	this->streamOpen = true;

	this->zipHandle = mz_zip_create();
	if (mz_zip_open(this->zipHandle, this->streamHandle, MZ_OPEN_MODE_READ) != MZ_OK) {
		return false; // No need to delete the stream, it's done when we destruct
	}
	this->zipOpen = true;

	return true;
}

void ZIP::closeZIP() {
	if (this->zipOpen) {
		mz_zip_close(this->zipHandle);
		mz_zip_delete(&this->zipHandle);
	}
	if (this->streamOpen) {
		mz_stream_os_close(this->streamHandle);
		mz_stream_os_delete(&this->streamHandle);
	}
}
