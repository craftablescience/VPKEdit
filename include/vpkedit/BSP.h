#pragma once

#include <vpkedit/PackFile.h>

namespace vpkedit {

class BSP : public PackFile {
public:
	/// Open a BSP pack lump
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const final;

	bool bake(const std::string& outputFolder_ /*= ""*/, const Callback& callback /*= nullptr*/) final;

protected:
	BSP(const std::string& fullFilePath_, PackFileOptions options_);

	Entry& addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) final;

private:
	VPKEDIT_REGISTER_PACKFILE_EXTENSION(".bsp", &BSP::open);
};

} // namespace vpkedit
