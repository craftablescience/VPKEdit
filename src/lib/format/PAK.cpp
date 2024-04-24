#include <vpkedit/format/PAK.h>

#include <filesystem>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/FileStream.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

PAK::PAK(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFile(fullFilePath_, options_) {
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

		pak->entries.insert(std::move(entry));

		if (callback) {
			callback(entry);
		}
	}

	return packFile;
}

std::optional<std::vector<std::byte>> PAK::readEntry(const Entry& entry) const {
	if (entry.unbaked) {
		return this->readUnbakedEntry(entry);
	}
	// It's baked into the file on disk
	FileStream stream{this->fullFilePath};
	if (!stream) {
		return std::nullopt;
	}
	stream.seekInput(entry.offset);
	return stream.readBytes(entry.length);
}

void PAK::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	entry.path = filename_;
	::normalizeSlashes(entry.path);
	if (!this->isCaseSensitive()) {
		::toLowerCase(entry.path);
	}

	entry.length = buffer.size();

	// Offset will be reset when it's baked
	entry.offset = 0;

	this->unbakedEntries.insert(std::move(entry));
}

bool PAK::bake(const std::string& outputDir_, const Callback& callback) {
	// Get the proper file output folder
	std::string outputDir = this->getBakeOutputDir(outputDir_);
	std::string outputPath = outputDir + '/' + this->getFilename() + "_test.pak";

	// Reconstruct data for ease of access
	std::vector<Entry*> entriesToBake;
	for (const auto& entry : this->entries) {
		entriesToBake.push_back(const_cast<Entry*>(&entry));
	}
	for (const auto& entry : this->unbakedEntries) {
		entriesToBake.push_back(const_cast<Entry*>(&entry));
	}

	// Read data before overwriting, we don't know if we're writing to ourself
	std::vector<std::byte> fileData;
	for (auto* entry : entriesToBake) {
		if (auto binData = this->readEntry(*entry)) {
			entry->offset = fileData.size();

			fileData.insert(fileData.end(), binData->begin(), binData->end());
		} else {
			entry->offset = 0;
			entry->length = 0;
		}
	}

	{
		FileStream stream{outputPath, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
		stream.seekOutput(0);

		// Signature
		stream.write(PAK_SIGNATURE);

		// Index and size of directory
		const std::uint32_t directoryIndex = sizeof(PAK_SIGNATURE) + sizeof(std::uint32_t) * 2;
		stream.write(directoryIndex);
		const std::uint32_t directorySize = entriesToBake.size() * 64;
		stream.write(directorySize);

		// Directory
		for (auto entry : entriesToBake) {
			stream.write(entry->path, PAK_FILENAME_MAX_SIZE, false);
			stream.write(static_cast<std::uint32_t>(entry->offset + directoryIndex + directorySize));
			stream.write(static_cast<std::uint32_t>(entry->length));

			if (callback) {
				callback(*entry);
			}
		}

		// File data
		stream.writeBytes(fileData);
	}

	// Clean up
	this->mergeUnbakedEntries();
	PackFile::setFullFilePath(outputDir);
	return true;
}

std::vector<Attribute> PAK::getSupportedEntryAttributes() const {
	using enum Attribute;
	return {LENGTH};
}
