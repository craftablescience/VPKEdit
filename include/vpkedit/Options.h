#pragma once

#include <cstdint>

namespace vpkedit {

/// VPK - Maximum preload data size in bytes
constexpr std::uint32_t VPK_MAX_PRELOAD_BYTES = 1024;

/// VPK - Chunk size in bytes (default is 200mb)
constexpr std::uint32_t VPK_DEFAULT_CHUNK_SIZE = 200 * 1024 * 1024;

struct PackFileOptions {
	/// Whether or not to allow uppercase letters in filenames. This affects all functions:
	/// if this value is false, filenames will be treated as case-insensitive.
	bool allowUppercaseLettersInFilenames = false;

	/// VPK - Version (ignored when opening an existing VPK)
	std::uint32_t vpk_version = 2;

	/// VPK - If this value is 0, chunks have an unlimited size (not controlled by the library)
	/// Chunk size is max 4gb, but since its not useful to have large chunks, try to keep the
	/// preferred chunk size around the default
	std::uint32_t vpk_preferredChunkSize = VPK_DEFAULT_CHUNK_SIZE;

	/// VPK - Controls generation of per-file MD5 hashes (only for VPK v2)
	bool vpk_generateMD5Entries = false;
};

struct EntryOptions {
	/// VPK - Save this entry to the directory VPK
	bool vpk_saveToDirectory = false;

	/// VPK - The amount in bytes of the file to preload. Maximum is controlled by VPK_MAX_PRELOAD_BYTES
	std::uint32_t vpk_preloadBytes = 0;
};

} // namespace vpkedit
