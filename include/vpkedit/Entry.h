#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <mz.h>

namespace vpkedit {

/// This class represents the metadata that a file has inside a PackFile.
/// It is used to access the file's data and read its properties
class Entry {
	friend class PackFile;

public:
	/// Path to this entry (e.g. "materials/cable.vmt")
	std::string path;
	/// Format-specific flags (PCK: File flags, ZIP/BSP: Compression type)
	std::uint32_t flags = 0;
	/// Length in bytes (in formats with compression, this is the uncompressed length)
	std::uint64_t length = 0;
	/// Offset, format-specific meaning - may be unused
	std::uint64_t offset = 0;
	/// If the format supports compression, this is the compressed length
	/// If compression is not supported or unused, this will remain 0
	std::uint64_t compressedLength = 0;
	/// CRC32 checksum - 0 if unused.
	/// Note that for GCF, this is actually an index into a checksum array and NOT a checksum
	std::uint32_t crc32 = 0;
	/// Used to check if entry is saved to disk
	bool unbaked = false;

	/// PCK - Each file has a 16-byte MD5 hash
	std::array<std::byte, 16> pck_md5{};

	/// VPK - Which VPK this entry is in
	std::uint16_t vpk_archiveIndex = 0;
	/// VPK - Preloaded data
	std::vector<std::byte> vpk_preloadedData;

	/// Returns the parent directory's path (e.g. "materials/cable.vmt" -> "materials")
	[[nodiscard]] std::string getParentPath() const;

	/// Returns the filename (e.g. "materials/cable.vmt" -> "cable.vmt")
	[[nodiscard]] std::string getFilename() const;

	/// Returns the file stem (e.g. "materials/cable.vmt" -> "cable")
	[[nodiscard]] std::string getStem() const;

	/// Returns the file extension without a period (e.g. "materials/cable.vmt" -> "vmt")
	[[nodiscard]] std::string getExtension() const;

private:
	/// The data attached to the unbaked entry, or the path to the file containing the unbaked entry's data
	std::variant<std::string, std::vector<std::byte>> unbakedData;
	/// Which one?
	bool unbakedUsingByteBuffer = false;

	Entry() = default;
};

} // namespace vpkedit
