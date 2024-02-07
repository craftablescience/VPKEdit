#pragma once

#include <cstdint>
#include <vector>

#include <vpkedit/PackFile.h>

namespace vpkedit {

constexpr std::string_view GCF_EXTENSION = ".gcf";

class GCF : public PackFileReadOnly {
public:
	struct Header {
		std::uint32_t dummy1;
		std::uint32_t dummy2;
		std::uint32_t gcfversion;
		std::uint32_t appid;
		std::uint32_t appversion;
		std::uint32_t dummy3;
		std::uint32_t dummy4;
		std::uint32_t filesize;
		std::uint32_t blocksize;
		std::uint32_t blockcount;
		std::uint32_t dummy5;
	};

	// second header with info about blocks inside the file
	// appears only ONCE not before every block!
	struct BlockHeader {
		std::uint32_t count;
		std::uint32_t used;
		std::uint32_t dummy1;
		std::uint32_t dummy2;
		std::uint32_t dummy3;
		std::uint32_t dummy4;
		std::uint32_t dummy5;
		std::uint32_t checksum;
	};

	struct Block {
		std::uint32_t entry_type;
		std::uint32_t file_data_offset;
		std::uint32_t file_data_size;
		std::uint32_t first_data_block_index;
		std::uint32_t next_block_entry_index;
		std::uint32_t prev_block_entry_index;
		std::uint32_t dir_index;
	};

	struct DirectoryHeader {
		std::uint32_t dummy1;
		std::uint32_t cacheid;
		std::uint32_t gcfversion;
		std::uint32_t itemcount;
		std::uint32_t filecount;
		std::uint32_t dummy2;
		std::uint32_t dirsize;
		std::uint32_t namesize;
		std::uint32_t info1count;
		std::uint32_t copycount;
		std::uint32_t localcount;
		std::uint32_t dummy3;
		std::uint32_t dummy4;
		std::uint32_t checksum;
	};

	struct DirectoryEntry {
		std::uint32_t nameoffset;
		std::uint32_t itemsize;
		std::uint32_t fileid;
		std::uint32_t dirtype;
		std::uint32_t parentindex;
		std::uint32_t nextindex;
		std::uint32_t firstindex;
	};

	struct DirectoryEntry2 {
		DirectoryEntry entry_real{};
		std::string filename;
	};

	struct DirectoryMapHeader {
		std::uint32_t dummy1;
		std::uint32_t dummy2;
	};

	struct DirectoryMapEntry {
		std::uint32_t firstblockindex;
	};

	struct DataBlockHeader {
		std::uint32_t appversion;
		std::uint32_t blockcount;
		std::uint32_t blocksize;
		std::uint32_t firstblockoffset;
		std::uint32_t used;
		std::uint32_t checksum;
	};

	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const override;

protected:
	GCF(const std::string& fullFilePath_, PackFileOptions options_);

	Header header{};
	BlockHeader blockheader{};
	std::vector<Block> blockdata{};
	std::vector<std::uint32_t> fragmap{};
	DirectoryHeader dirheader{};
	//std::vector<DirectoryEntry2> direntries {};
	std::vector<DirectoryMapEntry> dirmap_entries;
	DataBlockHeader datablockheader{};

private:
	VPKEDIT_REGISTER_PACKFILE_EXTENSION(GCF_EXTENSION, &GCF::open);
};

} // namespace vpkedit
