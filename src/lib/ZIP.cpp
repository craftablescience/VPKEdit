#include <vpkedit/ZIP.h>

#include <filesystem>

#include <MD5.h>
#include <vpkedit/detail/CRC.h>
#include <vpkedit/detail/Misc.h>

using namespace vpkedit;
using namespace vpkedit::detail;

ZIP::ZIP(const std::string& fullFilePath_, PackFileOptions options_)
		: PackFile(fullFilePath_, options_) {
	this->type = PackFileType::ZIP;
}

std::unique_ptr<PackFile> ZIP::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	if (!std::filesystem::exists(path)) {
		// File does not exist
		return nullptr;
	}

	auto* zip = new ZIP{path, options};
	auto packFile = std::unique_ptr<PackFile>(zip);

	// todo: load entries

	return packFile;
}

std::optional<std::vector<std::byte>> ZIP::readEntry(const Entry& entry) const {
	return std::nullopt; // todo: read entry
}

Entry& ZIP::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	return entry; // todo: add entry
}

bool ZIP::bake(const std::string& outputFolder_, const Callback& callback) {
	return true; // todo: save file
}
