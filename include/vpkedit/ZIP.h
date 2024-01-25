#pragma once

#include <vpkedit/PackFile.h>

namespace vpkedit {

class ZIP : public PackFile {
public:
	/// Open a ZIP file
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const final;

	bool bake(const std::string& outputFolder_ /*= ""*/, const Callback& callback /*= nullptr*/) final;

protected:
	ZIP(const std::string& fullFilePath_, PackFileOptions options_);

	Entry& addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) final;

private:
	VPKEDIT_REGISTER_PACKFILE_EXTENSION(".zip", &ZIP::open);
};

} // namespace vpkedit
