#include <vpkedit/format/GMA.h>

#include <filesystem>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/FileStream.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

GMA::GMA(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFile(fullFilePath_, options_) {
	this->type = PackFileType::GMA;
}

std::unique_ptr<PackFile> GMA::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* gma = new GMA{path, options};
	auto packFile = std::unique_ptr<PackFile>(gma);

	FileStream reader{gma->fullFilePath};
	reader.seekInput(0);

	reader.read(gma->header.signature);
	if (gma->header.signature != GMA_SIGNATURE) {
		// File is not a GMA
		return nullptr;
	}
	reader.read(gma->header.version);
	reader.read(gma->header.steamID);
	reader.read(gma->header.timestamp);
	reader.read(gma->header.requiredContent);
	reader.read(gma->header.addonName);
	reader.read(gma->header.addonDescription);
	reader.read(gma->header.addonAuthor);
	reader.read(gma->header.addonVersion);

	std::vector<Entry> entries;
	while (reader.read<std::uint32_t>() > 0) {
		Entry entry = createNewEntry();

		reader.read(entry.path);
		::normalizeSlashes(entry.path);
		if (!gma->isCaseSensitive()) {
			::toLowerCase(entry.path);
		}

		entry.length = reader.read<std::uint64_t>();
		reader.read(entry.crc32);

		entries.push_back(entry);
	}

	// At this point we've reached the file data section, calculate the offsets and then add the entries
	std::size_t offset = reader.tellInput();
	for (auto& entry : entries) {
		entry.offset = offset;
		offset += entry.length;
	}
	for (const auto& entry : entries) {
		gma->entries.insert(entry);

		if (callback) {
			callback(entry);
		}
	}

	return packFile;
}

std::vector<std::string> GMA::verifyEntryChecksums() const {
	return this->verifyEntryChecksumsUsingCRC32();
}

bool GMA::verifyFileChecksum() const {
	auto data = ::readFileData(this->fullFilePath);
	if (data.size() <= 4) {
		return true;
	}

	auto checksum = *reinterpret_cast<std::uint32_t*>(data.data() + data.size() - sizeof(std::uint32_t));
	data.pop_back();
	data.pop_back();
	data.pop_back();
	data.pop_back();
	return checksum == ::computeCRC32(data);
}

std::optional<std::vector<std::byte>> GMA::readEntry(const Entry& entry) const {
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

void GMA::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	entry.path = filename_;
	::normalizeSlashes(entry.path);
	if (!this->isCaseSensitive()) {
		::toLowerCase(entry.path);
	}

	entry.length = buffer.size();
	if (this->options.gma_writeCRCs) {
		entry.crc32 = ::computeCRC32(buffer);
	}

	// Offset will be reset when it's baked
	entry.offset = 0;

	this->unbakedEntries.insert(std::move(entry));
}

bool GMA::bake(const std::string& outputDir_, const Callback& callback) {
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

		// Header
		stream.write(this->header.signature);
		stream.write(this->header.version);
		stream.write(this->header.steamID);
		stream.write(this->header.timestamp);
		stream.write(this->header.requiredContent);
		stream.write(this->header.addonName);
		stream.write(this->header.addonDescription);
		stream.write(this->header.addonAuthor);
		stream.write(this->header.addonVersion);

		// File tree
		for (std::uint32_t i = 1; i <= entriesToBake.size(); i++) {
			stream.write(i);
			auto* entry = entriesToBake[i - 1];
			stream.write(entry->path);
			stream.write(entry->length);
			stream.write<std::uint32_t>(this->options.gma_writeCRCs ? entry->crc32 : 0);

			if (callback) {
				callback(*entry);
			}
		}
		stream.write(static_cast<std::uint32_t>(0));

		// Fix offsets
		std::size_t offset = stream.tellOutput();
		for (auto* entry : entriesToBake) {
			entry->offset = offset;
			offset += entry->length;
		}

		// File data
		stream.writeBytes(fileData);
	}

	// CRC of everything that's been written
	std::uint32_t crc = 0;
	if (this->options.gma_writeCRCs) {
		auto fileSize = std::filesystem::file_size(outputPath);
		FileStream stream{outputPath};
		stream.seekInput(0);
		crc = ::computeCRC32(stream.readBytes(fileSize));
	}
	{
		FileStream stream{outputPath, FILESTREAM_OPT_APPEND};
		stream.write(crc);
	}

	// Clean up
	this->mergeUnbakedEntries();
	PackFile::setFullFilePath(outputDir);
	return true;
}

std::vector<Attribute> GMA::getSupportedEntryAttributes() const {
	using enum Attribute;
	return {LENGTH, CRC32};
}

GMA::operator std::string() const {
	return PackFile::operator std::string() +
		" | Version v" + std::to_string(this->header.version) +
		" | Addon Name: \"" + this->header.addonName + "\"";
}
