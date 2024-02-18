#include <vpkedit/format/PAK.h>

#include <filesystem>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/FileStream.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

PAK::PAK(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFileReadOnly(fullFilePath_, options_) {
	this->type = PackFileType::PAK;
}

std::unique_ptr<PackFile> PAK::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* pak = new PAK{path, options};
	auto packFile = std::unique_ptr<PackFile>(pak);

	FileStream reader{pak->fullFilePath};
	reader.seekInput(0);

    if (auto signature = reader.read<std::int32_t>(); signature != PAK_SIGNATURE) {
        // File is not a PAK
        return nullptr;
    }

	auto directoryOffset = reader.read<std::uint32_t>();
	// Directory size / file entry size
	auto fileCount = reader.read<std::uint32_t>() / 64;

	reader.seekInput(directoryOffset);
	for (int i = 0; i < fileCount; i++) {
		Entry entry = createNewEntry();

		reader.read(entry.path, PAK_FILENAME_MAX_SIZE);
		::normalizeSlashes(entry.path);
		if (!pak->isCaseSensitive()) {
			::toLowerCase(entry.path);
		}

		entry.offset = reader.read<std::uint32_t>();
		entry.length = reader.read<std::uint32_t>();

		auto parentDir = std::filesystem::path(entry.path).parent_path().string();
		::normalizeSlashes(parentDir);
		if (!pak->isCaseSensitive()) {
			::toLowerCase(parentDir);
		}

		if (!pak->entries.contains(parentDir)) {
			pak->entries[parentDir] = {};
		}
		pak->entries[parentDir].push_back(entry);

		if (callback) {
			callback(parentDir, entry);
		}
	}

	return packFile;
}

std::optional<std::vector<std::byte>> PAK::readEntry(const Entry& entry) const {
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
	FileStream stream{this->fullFilePath};
	if (!stream) {
		return std::nullopt;
	}
	stream.seekInput(entry.offset);
	return stream.readBytes(entry.length);
}

std::vector<Attribute> PAK::getSupportedEntryAttributes() const {
	using enum Attribute;
	return {LENGTH};
}
