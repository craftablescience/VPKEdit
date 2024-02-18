#pragma once

#include <array>
#include <cstdint>

#include "ZIP.h"

namespace vpkedit {

constexpr std::int32_t BSP_SIGNATURE = 'V' + ('B' << 8) + ('S' << 16) + ('P' << 24);
constexpr std::int32_t BSP_LUMP_COUNT = 64;
constexpr std::int32_t BSP_LUMP_PAKFILE_INDEX = 40;
constexpr std::string_view BSP_EXTENSION = ".bsp";

class BSP : public ZIP {
#pragma pack(push, 1)
	struct Lump {
		/// Byte offset into file
		std::int32_t offset;
		/// Length of lump data
		std::int32_t length;
		/// Lump format version
		std::int32_t version;
		/// Uncompressed size, or 0
		std::int32_t fourCC;
	};

	struct Header {
		/// BSP_ID
		std::int32_t signature;
		/// Version of the BSP file
		std::int32_t version;
		/// Lump metadata
		std::array<Lump, BSP_LUMP_COUNT> lumps;
		/// Map version number
		std::int32_t mapRevision;
	};
#pragma pack(pop)

public:
	/// Open a BSP file
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] constexpr bool isCaseSensitive() const noexcept override {
		return false;
	}

	bool bake(const std::string& outputDir_ /*= ""*/, const Callback& callback /*= nullptr*/) override;

	[[nodiscard]] explicit operator std::string() const override;

protected:
	BSP(const std::string& fullFilePath_, PackFileOptions options_);

	/// If the lump is too big where it is, shift it to the end of the file, otherwise its fine
	void moveLumpToWritableSpace(int lumpToMove, int newSize);

	static const std::string TEMP_ZIP_PATH;

	Header header{};

private:
	VPKEDIT_REGISTER_PACKFILE_OPEN(BSP_EXTENSION, &BSP::open);
};

} // namespace vpkedit
