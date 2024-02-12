#include <vpkedit/GCF.h>

#include <algorithm>
#include <filesystem>
#include <tuple>

#include <vpkedit/detail/Adler32.h>
#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/FileStream.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

GCF::GCF(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFileReadOnly(fullFilePath_, options_) {
	this->type = PackFileType::GCF;
}

std::unique_ptr<PackFile> GCF::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	// TODO: Add v5 and perhaps v4 support

	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	// Create the pack file
	auto* gcf = new GCF{path, options};
	auto packFile = std::unique_ptr<PackFile>(gcf);

	// open file
	FileStream reader(gcf->fullFilePath);
	reader.seekInput(0);

	// we read the main header here (not the block header)
	reader.read(gcf->header);

	if (gcf->header.dummy1 != 1 && gcf->header.dummy2 != 1 && gcf->header.gcfversion != 6) {
		/**
		* This should NEVER occur when reading a "normal" gcf file.
		* It will only be false if
		* a) the gcf file isn't a gcf file
		* b) the gcf file is from a beta steam version that used a different format that there are no public docs for (v6 was used throughout 2004-2013)
		*/
		return nullptr;
	}

	uintmax_t real_size = std::filesystem::file_size(gcf->fullFilePath);
	if (real_size != gcf->header.filesize) {
		// again, this should never occur with a valid gcf file
		return nullptr;
	}

	reader.read(gcf->blockheader);
	if (gcf->blockheader.count != gcf->header.blockcount) {
		return nullptr;
	}

	// if you're having a headache reading the if statement below heres a quick explaination of what it actually does
	// it just adds all blockheader entries together and compares it against the checksum
	// if it's not the same then we bail out with a nullptr
	if (gcf->blockheader.count +
		gcf->blockheader.used +
		gcf->blockheader.dummy1 +
		gcf->blockheader.dummy2 +
		gcf->blockheader.dummy3 +
		gcf->blockheader.dummy4 +
		gcf->blockheader.dummy5 != gcf->blockheader.checksum) {
		return nullptr;
	}

	// block headers!!!!!!
	for (int i = 0; i < gcf->header.blockcount; i++) {
		Block& block = gcf->blockdata.emplace_back();
		reader.read(block);
	}

	// Fragmentation Map header
	// not worth keeping around after verifying stuff so no struct def
	// if you want one this is how one should look like
	/// struct FragMapHeader {
	///		std::uint32_t blockcount;
	///		std::uint32_t dummy0;
	///		std::uint32_t dummy1;
	///		std::uint32_t checksum;
	/// };
	// actually if we want to implement writing later this might be a better idea
	// if anyone wants to touch this piece of shit format again anyways

	auto blkcount = reader.read<std::uint32_t>();
	auto d1 = reader.read<std::uint32_t>();
	auto d2 = reader.read<std::uint32_t>();
	auto checksum = reader.read<std::uint32_t>();

	if (blkcount + d1 + d2 != checksum || blkcount != gcf->blockheader.count) {
		return nullptr;
	}

	// Fragmentation Map (list of dwords)

	for (int i = 0; i < blkcount; i++) {
		gcf->fragmap.push_back(reader.read<std::uint32_t>());
	}

	// Directory stuff starts here

	//Reading the header
	std::uint64_t temp = reader.tellInput();
	reader.read(gcf->dirheader);

	std::uint64_t diroffset = reader.tellInput() + (gcf->dirheader.itemcount * 28);
	std::uint64_t currentoffset;

	std::vector<DirectoryEntry2> direntries{};

	for (int i = 0; i < gcf->dirheader.itemcount; i++) {
		DirectoryEntry2& entry = direntries.emplace_back();
		reader.read(entry.entry_real);
		currentoffset = reader.tellInput();
		reader.seekInput(diroffset + entry.entry_real.nameoffset);
		reader.read(entry.filename);
		if (entry.entry_real.dirtype != 0) { // if not directory
			std::string dirname;
			DirectoryEntry2 current_filename_entry = entry;
			while (current_filename_entry.entry_real.parentindex != 0xffffffff) {
				current_filename_entry = direntries[current_filename_entry.entry_real.parentindex];
				dirname.insert(0, "/");
				dirname.insert(0, current_filename_entry.filename);
			}
			Entry gcfEntry = createNewEntry();
			::normalizeSlashes(dirname);
			if (!options.allowUppercaseLettersInFilenames) {
				::toLowerCase(dirname);
				::toLowerCase(entry.filename);
			}
			if (!gcf->entries.contains(dirname)) {
				//printf("dirname creation: %s\n", dirname.c_str());
				gcf->entries[dirname] = {};
			}
			gcfEntry.length = entry.entry_real.itemsize;
			gcfEntry.path = dirname;
			gcfEntry.path += dirname.empty() ? "" : "/";
			gcfEntry.path += entry.filename;
			gcfEntry.crc32 = entry.entry_real.fileid; // INDEX INTO THE CHECKSUM MAP VECTOR NOT CRC32!!!
			gcfEntry.offset = i; // THIS IS THE STRUCT INDEX NOT SOME OFFSET!!!
			//printf("%s\n", vpkedit_entry.path.c_str());
			gcf->entries[dirname].push_back(gcfEntry);
			//printf("dir %s file %s\n", dirname.c_str(), entry.filename.c_str());

			if (callback) {
				callback(dirname, gcfEntry);
			}
		}
		reader.seekInput(currentoffset);
	}

	// Directory Map

	// Directory Map header
	reader.seekInput(temp + gcf->dirheader.dirsize);

	//auto dmap = reader.read<DirectoryMapHeader>();
	reader.skipInput<DirectoryMapHeader>();

	// Directory Map entries
	for (int i = 0; i < gcf->dirheader.itemcount; i++) {
		DirectoryMapEntry& entry = gcf->dirmap_entries.emplace_back();
		reader.read(entry);
	}

	// Checksum header
	//auto dummy0 = reader.read<std::uint32_t>();
	reader.skipInput<std::uint32_t>();
	auto checksumsize = reader.read<std::uint32_t>();
	std::size_t checksums_start = reader.tellInput();

	//printf("checksums start: %llu\n", checksums_start);
	//printf("%lu %lu %lu %lu\n", gcf->header.blockcount, gcf->blockheader.used, gcf->header.appid, gcf->header.appversion);
	// map header

	ChecksumMapHeader chksummapheader = reader.read<ChecksumMapHeader>();
	if (chksummapheader.dummy1 != 0x14893721 || chksummapheader.dummy2 != 0x1) {
		return nullptr;
	}

	//printf("%lu %lu\n", chksummapheader.checksum_count, chksummapheader.item_count);

	for (int i = 0; i < chksummapheader.item_count; i++) {
		auto& cur_entry = gcf->chksum_map.emplace_back();
		reader.read(cur_entry);
	}

	for (int i = 0; i < chksummapheader.checksum_count; i++) {
		auto& currentChecksum = gcf->checksums.emplace_back();
		reader.read(currentChecksum);
	}
	//printf("current pos: %llu, block header: %llu should be: %llu", reader.tellInput(), reader.tellInput() + 0x80, checksums_start + checksumsize);
	// TODO: check the checksum RSA signature... later.. if ever...

	reader.seekInput(checksums_start + checksumsize);

	reader.read(gcf->datablockheader);
	return packFile;
}

std::optional<std::vector<std::byte>> GCF::readEntry(const Entry& entry) const {
	if (entry.unbaked) {
		// Get the stored data
		for (const auto& [unbakedEntryDir, unbakedEntryList] : this->unbakedEntries) {
			for (const Entry& unbakedEntry : unbakedEntryList) {
				if (unbakedEntry.path == entry.path) {
					std::vector<std::byte> unbakedData;
					if (isEntryUnbakedUsingByteBuffer(unbakedEntry)) {
						unbakedData = std::get<std::vector<std::byte>>(getEntryUnbakedData(unbakedEntry));
					}
					else {
						unbakedData = ::readFileData(std::get<std::string>(getEntryUnbakedData(unbakedEntry)));
					}
					return unbakedData;
				}
			}
		}
		return std::nullopt;
	}

	std::vector<std::byte> filedata;
	if (entry.length == 0) {
		// don't bother
		return filedata;
	}

	std::uint32_t dir_index = entry.offset;
	//printf(" extracting file: %s\n", entry.path.c_str());

	std::vector<Block> toread;
	for (const auto& v : this->blockdata) {
		if (v.dir_index == dir_index) {
			toread.push_back(v);
		}
	}

	std::sort(toread.begin(), toread.end(), [](Block lhs, Block rhs) {
		return (lhs.file_data_offset < rhs.file_data_offset);
		}
	);

	if (toread.empty()) {
		//printf("could not find any directory index for %lu", entry.vpk_offset);
		return std::nullopt;
	}

	FileStream stream{this->fullFilePath};
	if (!stream) {
		//printf("!stream\n");
		return std::nullopt;
	}

	std::uint64_t remaining = entry.length;

	for (const auto& block : toread) {
		std::uint32_t currindex = block.first_data_block_index;
		while (currindex <= this->blockheader.count) {
			std::uint64_t curfilepos = static_cast<std::uint64_t>(this->datablockheader.firstblockoffset) + (static_cast<std::uint64_t>(0x2000) * static_cast<std::uint64_t>(currindex));
			stream.seekInput(curfilepos);
			//printf("off %lli block %lu toread %lli should be %llu\n", stream.tellInput(), currindex, remaining, curfilepos);
			std::uint32_t toreadAmt = std::min(remaining, static_cast<std::uint64_t>(0x2000));
			auto streamvec = stream.readBytes(toreadAmt);
			filedata.insert(filedata.end(), streamvec.begin(), streamvec.end());
			remaining -= toreadAmt;
			currindex = this->fragmap[currindex];
			//printf("curridx now: %lu\n", currindex);
		}
	}

	return filedata;
}

std::vector<std::string> GCF::verifyEntryChecksums() const {
	std::vector<std::string> bad;
	for (const auto& entryList : this->entries) {
		for (const auto& entry : entryList.second) {
			auto bytes = this->readEntry(entry);
			if (!bytes || bytes->empty()) {
				continue;
			}
			std::size_t tocheck = bytes->size();
			std::uint32_t idx = entry.crc32;
			std::uint32_t count = this->chksum_map[idx].count;
			std::uint32_t checksumstart = this->chksum_map[idx].firstindex;
			for (int i = 0; i < count; i++) {
				std::uint32_t csum = this->checksums[checksumstart + i];
				std::size_t toread = std::min(static_cast<std::size_t>(0x8000), tocheck);
				const auto* data = bytes->data() + (i * 0x8000);
				std::uint32_t checksum = ::computeCRC(data, toread) ^ ::computeAdler32(data, toread);
				if (checksum != csum) {
					bad.push_back(entry.path);
				}
				tocheck -= toread;
			}
		}
	}
	return bad;
}
