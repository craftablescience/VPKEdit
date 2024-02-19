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
		auto parentDir = std::filesystem::path(entry.path).parent_path().string();
		::normalizeSlashes(parentDir);
		if (!gma->isCaseSensitive()) {
			::toLowerCase(parentDir);
		}

		if (!gma->entries.contains(parentDir)) {
			gma->entries[parentDir] = {};
		}
		gma->entries[parentDir].push_back(entry);

		if (callback) {
			callback(parentDir, entry);
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

Entry& GMA::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	auto filename = filename_;
	if (!this->isCaseSensitive()) {
		::toLowerCase(filename);
	}
	auto [dir, name] = ::splitFilenameAndParentDir(filename);

	entry.path = filename;
	entry.length = buffer.size();
	if (this->options.gma_writeCRCs) {
		entry.crc32 = ::computeCRC32(buffer);
	}

	// Offset will be reset when it's baked
	entry.offset = 0;

	if (!this->unbakedEntries.contains(dir)) {
		this->unbakedEntries[dir] = {};
	}
	this->unbakedEntries.at(dir).push_back(entry);
	return this->unbakedEntries.at(dir).back();
}

bool GMA::bake(const std::string& outputDir_, const Callback& callback) {
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
			fileData.insert(fileData.end(), binData->begin(), binData->end());
		} else {
			// Offsets are fixed later
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
				callback(entry->getParentPath(), *entry);
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
