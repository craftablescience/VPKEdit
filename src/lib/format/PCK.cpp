#include <vpkedit/format/PCK.h>

#include <filesystem>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/FileStream.h>
#include <vpkedit/detail/MD5.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

constexpr int PCK_DIRECTORY_STRING_PADDING = 4;
constexpr int PCK_FILE_DATA_PADDING = 16;

namespace {

/*
 * This function is modified from Godot Engine code, licensed under the MIT License.
 * Copyright (c) 2014-present Godot Engine contributors.
 * Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.
 * https://github.com/godotengine/godot/blob/99ff024f78f65ba0bc54fb409cfeca43ba2008fe/core/io/pck_packer.cpp#L39
 */
std::size_t getPadding(int alignment, int n) {
	if (const int rest = n % alignment; rest > 0) {
		return alignment - rest;
	}
	return 0;
}

} // namespace

PCK::PCK(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFile(fullFilePath_, options_) {
	this->type = PackFileType::PCK;
}

std::unique_ptr<PackFile> PCK::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* pck = new PCK{path, options};
	auto packFile = std::unique_ptr<PackFile>(pck);

	FileStream reader{pck->fullFilePath};
	reader.seekInput(0);

	if (auto signature = reader.read<std::uint32_t>(); signature != PCK_SIGNATURE) {
		// PCK might be embedded
		reader.seekInput(-sizeof(std::uint32_t), std::ios::end);
		if (auto endSignature = reader.read<std::uint32_t>(); endSignature != PCK_SIGNATURE) {
			return nullptr;
		}

		reader.seekInput(-(sizeof(std::uint32_t) + sizeof(std::uint64_t)), std::ios::cur);
		auto distanceIntoFile = reader.read<std::uint64_t>();

		reader.seekInput(-(distanceIntoFile + sizeof(std::uint64_t)), std::ios::cur);
		if (auto startSignature = reader.read<std::uint32_t>(); startSignature != PCK_SIGNATURE) {
			return nullptr;
		}

		pck->startOffset = reader.tellInput() - sizeof(std::uint32_t);
	}

	reader.read(pck->header.packVersion);
	reader.read(pck->header.godotVersionMajor);
	reader.read(pck->header.godotVersionMinor);
	reader.read(pck->header.godotVersionPatch);

	pck->header.flags = FLAG_NONE;
	std::size_t extraEntryContentsOffset = 0;
	if (pck->header.packVersion > 1) {
		pck->header.flags = reader.read<FlagsV2>();
		extraEntryContentsOffset = reader.read<std::uint64_t>();
	}

	if (pck->header.flags & FLAG_ENCRYPTED) {
		// File directory is encrypted
		return nullptr;
	}
	if (pck->header.flags & FLAG_RELATIVE_FILE_DATA) {
		extraEntryContentsOffset += pck->startOffset;
		pck->header.flags = static_cast<FlagsV2>(pck->header.flags & ~FLAG_RELATIVE_FILE_DATA);
	}

	// Reserved
	reader.skipInput<std::int32_t>(16);

	// Directory
	auto fileCount = reader.read<std::uint32_t>();
	for (std::uint32_t i = 0; i < fileCount; i++) {
		Entry entry = createNewEntry();

		entry.path = reader.readString(reader.read<std::uint32_t>());
		if (entry.path.starts_with(PCK_PATH_PREFIX)) {
			entry.path = entry.path.substr(PCK_PATH_PREFIX.length());
		}
		::normalizeSlashes(entry.path);
		if (!pck->isCaseSensitive()) {
			::toLowerCase(entry.path);
		}

		entry.offset = reader.read<std::uint64_t>() + extraEntryContentsOffset;
		entry.length = reader.read<std::uint64_t>();
		entry.pck_md5 = reader.readBytes<16>();

		if (pck->header.packVersion > 1) {
			entry.flags = reader.read<std::uint32_t>();
		}

		auto parentDir = std::filesystem::path(entry.path).parent_path().string();
		::normalizeSlashes(parentDir);
		if (!pck->entries.contains(parentDir)) {
			pck->entries[parentDir] = {};
		}
		pck->entries[parentDir].push_back(entry);

		if (callback) {
			callback(parentDir, entry);
		}
	}

	// File data
	pck->dataOffset = reader.tellInput();

	return packFile;
}

std::optional<std::vector<std::byte>> PCK::readEntry(const Entry& entry) const {
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
	if (entry.flags & FLAG_ENCRYPTED) {
		// File is encrypted
		return std::nullopt;
	}

	FileStream stream{this->fullFilePath};
	if (!stream) {
		return std::nullopt;
	}
	stream.seekInput(entry.offset);
	return stream.readBytes(entry.length);
}

Entry& PCK::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	auto filename = filename_;
	if (!this->isCaseSensitive()) {
		::toLowerCase(filename);
	}
	auto [dir, name] = ::splitFilenameAndParentDir(filename);

	entry.path = filename;
	entry.length = buffer.size();
	entry.pck_md5 = ::computeMD5(buffer);

	// Offset will be reset when it's baked
	entry.offset = 0;

	if (!this->unbakedEntries.contains(dir)) {
		this->unbakedEntries[dir] = {};
	}
	this->unbakedEntries.at(dir).push_back(entry);
	return this->unbakedEntries.at(dir).back();
}

bool PCK::bake(const std::string& outputDir_, const Callback& callback) {
	// Get the proper file output folder
	std::string outputDir = this->getBakeOutputDir(outputDir_);
	std::string outputPath = outputDir + '/' + this->getFilename();

	// Reconstruct data for ease of access
	std::vector<Entry*> entriesToBake;
	for (auto& [entryDir, entryList] : this->entries) {
		for (auto& entry : entryList) {
			entriesToBake.push_back(&entry);
		}
	}
	for (auto& [entryDir, entryList] : this->unbakedEntries) {
		for (auto& entry : entryList) {
			entriesToBake.push_back(&entry);
		}
	}

	// Read data before overwriting, we don't know if we're writing to ourself
	std::vector<std::byte> fileData;
	for (auto* entry : entriesToBake) {
		if (auto binData = this->readEntry(*entry)) {
			entry->offset = fileData.size();

			fileData.insert(fileData.end(), binData->begin(), binData->end());
			const auto padding = ::getPadding(PCK_FILE_DATA_PADDING, static_cast<int>(entry->length));
			for (int i = 0; i < padding; i++) {
				fileData.push_back(static_cast<std::byte>(0));
			}
		} else {
			entry->offset = 0;
			entry->length = 0;
		}
	}

	// If this is an embedded pck, read the executable data first
	std::vector<std::byte> exeData;
	if (this->startOffset > 0) {
		FileStream stream{this->fullFilePath};
		if (!stream) {
			return false;
		}
		stream.seekInput(0);
		exeData = stream.readBytes(this->startOffset);
	}

	// Write data
	{
		FileStream stream{outputPath, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
		stream.seekOutput(0);

		if (!exeData.empty()) {
			stream.writeBytes(exeData);
		}

		// Signature
		stream.write(PCK_SIGNATURE);

		// Header
		stream.write(this->header.packVersion);
		stream.write(this->header.godotVersionMajor);
		stream.write(this->header.godotVersionMinor);
		stream.write(this->header.godotVersionPatch);

		if (this->header.packVersion > 1) {
			stream.write(this->header.flags);
			stream.write<std::uint64_t>(0);
		}

		// Reserved
		stream.write(std::array<std::int32_t, 16>{});

		// Directory start
		stream.write(static_cast<std::uint32_t>(entriesToBake.size()));

		// Dry-run to get the length of the directory section
		this->dataOffset = stream.tellOutput();
		for (auto* entry : entriesToBake) {
			const auto entryPath = std::string{PCK_PATH_PREFIX} + entry->path;
			const auto padding = ::getPadding(PCK_DIRECTORY_STRING_PADDING, static_cast<int>(entryPath.length()));
			this->dataOffset +=
					sizeof(std::uint32_t) +        // Path length
					entryPath.length() + padding + // Path
					(sizeof(std::size_t) * 2) +    // Offset, Length
					(sizeof(std::byte) * 16);      // MD5

			if (this->header.packVersion > 1) {
				this->dataOffset += sizeof(std::uint32_t); // Flags
			}
		}

		// Directory
		for (auto* entry : entriesToBake) {
			const auto entryPath = std::string{PCK_PATH_PREFIX} + entry->path;
			const auto padding = ::getPadding(PCK_DIRECTORY_STRING_PADDING, static_cast<int>(entryPath.length()));
			stream.write(static_cast<std::uint32_t>(entryPath.length() + padding));
			stream.write(entryPath, entryPath.length() + padding, false);

			entry->offset += this->dataOffset;
			stream.write(entry->offset);
			stream.write(entry->length);
			stream.writeBytes(entry->pck_md5);

			if (this->header.packVersion > 1) {
				stream.write(entry->flags);
			}

			if (callback) {
				callback(entry->getParentPath(), *entry);
			}
		}

		// File data
		stream.writeBytes(fileData);

		// Write offset to start
		if (this->startOffset > 0) {
			stream.write(stream.tellOutput() - startOffset);
			stream.write(PCK_SIGNATURE);
		}
	}

	// Clean up
	this->mergeUnbakedEntries();
	PackFile::setFullFilePath(outputDir);
	return true;
}

std::vector<Attribute> PCK::getSupportedEntryAttributes() const {
	using enum Attribute;
	return {LENGTH, PCK_MD5};
}

PCK::operator std::string() const {
	auto out = PackFile::operator std::string() +
		" | Version v" + std::to_string(this->header.packVersion) +
		" | Godot Version v" + std::to_string(this->header.godotVersionMajor) + '.' + std::to_string(this->header.godotVersionMinor) + '.' + std::to_string(this->header.godotVersionPatch);
	if (this->startOffset > 0) {
		out += " | Embedded";
	}
	if (this->header.flags & FLAG_ENCRYPTED) {
		out += " | Encrypted";
	}
	return out;
}
