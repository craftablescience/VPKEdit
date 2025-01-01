#include "Folder.h"

#include <filesystem>

#include <sourcepp/FS.h>

using namespace sourcepp;
using namespace vpkpp;

std::unique_ptr<PackFile> Folder::open(const std::string& path, const EntryCallback& callback) {
	auto* folder = new Folder{path};
	std::unique_ptr<PackFile> packFile{folder};

	std::error_code ec;
	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator{path, std::filesystem::directory_options::skip_permission_denied, ec}) {
		if (!dirEntry.is_regular_file(ec)) {
			continue;
		}
		ec.clear();
		auto entryPath = std::filesystem::relative(dirEntry.path(), path, ec).string();
		if (ec || entryPath.empty()) {
			continue;
		}

		Entry entry = createNewEntry();
		entry.length = std::filesystem::file_size(dirEntry.path(), ec);
		folder->entries.insert(folder->cleanEntryPath(entryPath), entry);
	}

	return packFile;
}

std::optional<std::vector<std::byte>> Folder::readEntry(const std::string& path_) const {
	auto path = this->fullFilePath + '/' + this->cleanEntryPath(path_);
	std::error_code ec;
	if (!std::filesystem::is_regular_file(path, ec)) {
		return std::nullopt;
	}
	return fs::readFileBuffer(path);
}

Attribute Folder::getSupportedEntryAttributes() const {
	using enum Attribute;
	return LENGTH;
}
