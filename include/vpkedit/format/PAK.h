#pragma once

#include <cstdint>

#include "../PackFile.h"

namespace vpkedit {

constexpr std::int8_t PAK_FILENAME_MAX_SIZE = 56;
constexpr std::int32_t PAK_SIGNATURE = 'P' + ('A' << 8) + ('C' << 16) + ('K' << 24);
constexpr std::string_view PAK_EXTENSION = ".pak";

class PAK : public PackFileReadOnly {
public:
	/// Open a PAK file
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const override;

	[[nodiscard]] std::vector<Attribute> getSupportedEntryAttributes() const override;

protected:
	PAK(const std::string& fullFilePath_, PackFileOptions options_);

private:
	VPKEDIT_REGISTER_PACKFILE_OPEN(PAK_EXTENSION, &PAK::open);
};

} // namespace vpkedit
