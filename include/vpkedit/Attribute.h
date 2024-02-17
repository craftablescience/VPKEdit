#pragma once

namespace vpkedit {

enum class Attribute : int {
	//PATH, // Not included because its implied
	LENGTH = 0,
	VPK_PRELOADED_DATA_LENGTH,
	VPK_ARCHIVE_INDEX,
	CRC32,
	PCK_MD5,
	ATTRIBUTE_COUNT,
};

} // namespace vpkedit
