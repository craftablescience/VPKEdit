#include <vpkedit/format/PCK.h>

#include <filesystem>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/FileStream.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

PCK::PCK(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFileReadOnly(fullFilePath_, options_) {
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
		if (auto endSignature = reader.read<std::uint32_t>(); endSignature != PCK_SIGNATURE) {
			return nullptr;
		}

		pck->embedded = true;
		pck->startOffset = reader.tellInput() - sizeof(std::uint32_t);
	}

	reader.read(pck->header.packVersion);
	reader.read(pck->header.godotVersionMajor);
	reader.read(pck->header.godotVersionMinor);
	reader.read(pck->header.godotVersionPatch);

	pck->header.flags = 0;
	std::size_t extraEntryContentsOffset = 0;
	if (pck->header.packVersion > 1) {
		pck->header.flags = reader.read<std::uint32_t>();
		pck->entryContentsOffset = reader.tellInput();
		extraEntryContentsOffset = reader.read<std::uint64_t>();
	}

	if (pck->header.flags & 0x1) {
		// File directory is encrypted
		return nullptr;
	}

	// Reserved
	reader.skipInput<std::int32_t>(16);

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
	if (entry.flags & 0x1) {
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
	return out;
}
