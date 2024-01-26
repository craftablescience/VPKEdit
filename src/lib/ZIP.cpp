#include <vpkedit/ZIP.h>

#include <filesystem>

#include <MD5.h>
#include <mz.h>
#include <mz_strm.h>
#include <mz_strm_os.h>
#include <mz_zip.h>
#include <vpkedit/detail/CRC.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

ZIP::ZIP(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFile(fullFilePath_, options_) {
	this->type = PackFileType::ZIP;
}

ZIP::~ZIP() {
	if (this->zipOpen) {
		mz_zip_close(this->zipHandle);
		mz_zip_delete(&this->zipHandle);
	}
	if (this->streamOpen) {
		mz_stream_os_close(this->streamHandle);
		mz_stream_os_delete(&this->streamHandle);
	}
}

std::unique_ptr<PackFile> ZIP::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* zip = new ZIP{path, options};
	auto packFile = std::unique_ptr<PackFile>(zip);

	zip->streamHandle = mz_stream_os_create();
	if (mz_stream_open(zip->streamHandle, path.c_str(), MZ_OPEN_MODE_READ) != MZ_OK) {
		return nullptr;
	}
	zip->streamOpen = true;

	zip->zipHandle = mz_zip_create();
	if (mz_zip_open(zip->zipHandle, zip->streamHandle, MZ_OPEN_MODE_READ) != MZ_OK) {
		mz_stream_os_close(zip->streamHandle);
		mz_stream_os_delete(&zip->streamHandle);
		return nullptr;
	}
	zip->zipOpen = true;

	for (auto code = mz_zip_goto_first_entry(zip->zipHandle); code == MZ_OK; code = mz_zip_goto_next_entry(zip->zipHandle)) {
		mz_zip_file* fileInfo = nullptr;
		code = mz_zip_entry_get_info(zip->zipHandle, &fileInfo);
		if (code != MZ_OK) {
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

		entry.length = fileInfo->uncompressed_size;
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
	entry.crc32 = computeCRC(buffer); // todo: may not be needed
	entry.length = buffer.size();

	if (!this->unbakedEntries.contains(dir)) {
		this->unbakedEntries[dir] = {};
	}
	this->unbakedEntries.at(dir).push_back(entry);
	return this->unbakedEntries.at(dir).back();
}

bool ZIP::bake(const std::string& outputFolder_, const Callback& callback) {
	return true; // todo: save file
}
