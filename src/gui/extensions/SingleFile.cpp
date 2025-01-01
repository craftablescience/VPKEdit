#include "SingleFile.h"

#include <filesystem>

#include <sourcepp/FS.h>

using namespace sourcepp;
using namespace vpkpp;

std::unique_ptr<PackFile> SingleFile::open(const std::string& path, const EntryCallback& callback) {
	auto* singleFile = new SingleFile{path};
	std::unique_ptr<PackFile> packFile{singleFile};

	std::error_code ec;
	if (!std::filesystem::is_regular_file(path, ec)) {
		return nullptr;
	}

	auto entryPath = std::filesystem::path{path}.filename().string();

	Entry entry = createNewEntry();
	entry.length = std::filesystem::file_size(path, ec);
	singleFile->entries.insert(singleFile->cleanEntryPath(entryPath), entry);

	return packFile;
}

std::optional<std::vector<std::byte>> SingleFile::readEntry(const std::string& path_) const {
	// Do this so it can load files next to itself, for MDL preview
	auto path = (std::filesystem::path{this->fullFilePath}.parent_path() / this->cleanEntryPath(path_)).string();
	std::error_code ec;
	if (!std::filesystem::is_regular_file(path, ec)) {
		return std::nullopt;
	}
	return fs::readFileBuffer(path);
}
