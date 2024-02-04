#pragma once

#include <cstdint>

#include <vpkedit/detail/FileStream.h>
#include <vpkedit/PackFile.h>

namespace vpkedit {

constexpr std::int32_t GMA_ID = 'G' + ('M' << 8) + ('A' << 16) + ('D' << 24);

class GMA : public PackFile {
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
	VPKEDIT_REGISTER_PACKFILE_EXTENSION(".gma", &GMA::open);
};

} // namespace vpkedit
