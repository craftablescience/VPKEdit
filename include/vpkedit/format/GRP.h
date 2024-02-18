#pragma once

#include <cstdint>

#include "../PackFile.h"

namespace vpkedit {

constexpr std::int8_t GRP_FILENAME_MAX_SIZE = 12;
constexpr std::string_view GRP_SIGNATURE = "KenSilverman";
constexpr std::string_view GRP_EXTENSION = ".grp";

class GRP : public PackFile {
public:
	/// Open a GRP file
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const override;

	bool bake(const std::string& outputDir_ /*= ""*/, const Callback& callback /*= nullptr*/) override;

	[[nodiscard]] std::vector<Attribute> getSupportedEntryAttributes() const override;

protected:
	GRP(const std::string& fullFilePath_, PackFileOptions options_);

	Entry& addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) override;

private:
	VPKEDIT_REGISTER_PACKFILE_OPEN(GRP_EXTENSION, &GRP::open);
};

} // namespace vpkedit
