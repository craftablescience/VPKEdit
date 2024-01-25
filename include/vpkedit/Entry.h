#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace vpkedit {

/// This class represents the metadata that a file has inside a PackFile.
/// It is used to access the file's data and read its properties
class Entry {
	friend class PackFile;

public:
	/// File name of this entry (e.g. "cable.vmt")
	std::string filename;
	/// Length in bytes
	std::uint32_t length = 0;
	/// Offset in the VPK
	std::uint32_t offset = 0;
	/// Used to check if entry is saved to disk
	bool unbaked = false;

	/// VPK - CRC32 checksum
	std::uint32_t vpk_crc32 = 0;
	/// VPK - Which VPK this entry is in
	std::uint16_t vpk_archiveIndex = 0;
	/// VPK - Preloaded data
	std::vector<std::byte> vpk_preloadedData;

	/// Returns the file stem (e.g. "cable.vmt" -> "cable")
	[[nodiscard]] std::string getStem() const;

	/// Returns the file extension without a period (e.g. "cable.vmt" -> "vmt")
	[[nodiscard]] std::string getExtension() const;

private:
	/// The data attached to the unbaked entry, or the path to the file containing the unbaked entry's data
	std::variant<std::string, std::vector<std::byte>> unbakedData;
	/// Which one?
	bool unbakedUsingByteBuffer = false;

	Entry() = default;
};

} // namespace vpkedit
