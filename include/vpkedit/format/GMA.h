#pragma once

#include <cstdint>

#include "../PackFile.h"

namespace vpkedit {

constexpr std::int32_t GMA_SIGNATURE = 'G' + ('M' << 8) + ('A' << 16) + ('D' << 24);
constexpr std::string_view GMA_EXTENSION = ".gma";

class GMA : public PackFile {
protected:
	struct Header {
		std::uint32_t signature;
		std::uint8_t version;
		std::uint64_t steamID;
		std::uint64_t timestamp;
		std::string requiredContent;
		std::string addonName;
		std::string addonDescription;
		std::string addonAuthor;
		std::int32_t addonVersion;
	};

public:
	/// Open a GMA file
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const override;

	bool bake(const std::string& outputDir_ /*= ""*/, const Callback& callback /*= nullptr*/) override;

protected:
	GMA(const std::string& fullFilePath_, PackFileOptions options_);

	Entry& addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) override;

	Header header{};

private:
	VPKEDIT_REGISTER_PACKFILE_EXTENSION(GMA_EXTENSION, &GMA::open);
};

} // namespace vpkedit
