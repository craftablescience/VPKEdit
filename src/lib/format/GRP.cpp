#include <vpkedit/format/GRP.h>

#include <filesystem>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/FileStream.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

GRP::GRP(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFile(fullFilePath_, options_) {
	this->type = PackFileType::GRP;
}

std::unique_ptr<PackFile> GRP::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* grp = new GRP{path, options};
	auto packFile = std::unique_ptr<PackFile>(grp);

	FileStream reader{grp->fullFilePath};
	reader.seekInput(0);

	auto signature = reader.readBytes<GRP_SIGNATURE.length()>();
	for (int i = 0; i < signature.size(); i++) {
		if (static_cast<unsigned char>(signature[i]) != GRP_SIGNATURE[i]) {
			// File is not a GRP
			return nullptr;
		}
	}

	auto fileCount = reader.read<std::uint32_t>();

	std::vector<Entry> entries;
	for (int i = 0; i < fileCount; i++) {
		Entry entry = createNewEntry();

		reader.read(entry.path, GRP_FILENAME_MAX_SIZE);
		::normalizeSlashes(entry.path);
		if (!grp->isCaseSensitive()) {
			::toLowerCase(entry.path);
		}

		entry.length = reader.read<std::uint32_t>();

		entries.push_back(entry);
	}

	// At this point we've reached the file data section, calculate the offsets and then add the entries
	std::size_t offset = reader.tellInput();
	for (auto& entry : entries) {
		entry.offset = offset;
		offset += entry.length;

		grp->entries.insert(std::move(entry));

		if (callback) {
			callback(entry);
		}
	}

	return packFile;
}

std::optional<std::vector<std::byte>> GRP::readEntry(const Entry& entry) const {
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

void GRP::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	auto filename = filename_;
	if (!this->isCaseSensitive()) {
		::toLowerCase(filename);
	}

	entry.path = filename;
	entry.length = buffer.size();

	// Offset will be reset when it's baked
	entry.offset = 0;

	this->unbakedEntries.insert(std::move(entry));
}

bool GRP::bake(const std::string& outputDir_, const Callback& callback) {
	// Get the proper file output folder
	std::string outputDir = this->getBakeOutputDir(outputDir_);
	std::string outputPath = outputDir + '/' + this->getFilename();

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
			fileData.insert(fileData.end(), binData->begin(), binData->end());
		} else {
			entry->length = 0;
		}
	}

	{
		FileStream stream{outputPath, FILESTREAM_OPT_WRITE | FILESTREAM_OPT_TRUNCATE | FILESTREAM_OPT_CREATE_IF_NONEXISTENT};
		stream.seekOutput(0);

		// Signature
		stream.write(std::string{GRP_SIGNATURE}, false);

		// Number of files
		stream.write(static_cast<std::uint32_t>(entriesToBake.size()));

		// File tree
		for (auto entry : entriesToBake) {
			stream.write(entry->path, GRP_FILENAME_MAX_SIZE, false);
			stream.write(static_cast<std::uint32_t>(entry->length));

			if (callback) {
				callback(*entry);
			}
		}

		// Fix offsets
		std::size_t offset = stream.tellOutput();
		for (auto* entry : entriesToBake) {
			entry->offset = offset;
			offset += entry->length;
		}

		// File data
		stream.writeBytes(fileData);
	}

	// Clean up
	this->mergeUnbakedEntries();
	PackFile::setFullFilePath(outputDir);
	return true;
}

std::vector<Attribute> GRP::getSupportedEntryAttributes() const {
	using enum Attribute;
	return {LENGTH};
}
