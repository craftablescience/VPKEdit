#pragma once

#include <array>
#include <cstdint>

#include <vpkedit/detail/FileStream.h>
#include <vpkedit/ZIP.h>

namespace vpkedit {

constexpr std::int32_t BSP_ID = 'V' + ('B' << 8) + ('S' << 16) + ('P' << 24);
constexpr std::int32_t BSP_LUMP_COUNT = 64;
constexpr std::int32_t BSP_LUMP_PACK_INDEX = 40;

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

	bool bake(const std::string& outputFolder_ /*= ""*/, const Callback& callback /*= nullptr*/) override;

protected:
	BSP(const std::string& fullFilePath_, PackFileOptions options_);

	static const std::string BSP_TEMP_ZIP_PATH;

	Header header{};

	detail::FileStream reader;

private:
	VPKEDIT_REGISTER_PACKFILE_EXTENSION(".bsp", &BSP::open);
};

} // namespace vpkedit
