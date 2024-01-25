#include <vpkedit/PackFile.h>

#include <filesystem>
#include <utility>

#include <vpkedit/detail/Misc.h>
#include <vpkedit/VPK.h>

using namespace vpkedit;
using namespace vpkedit::detail;

PackFile::PackFile(std::string fullFilePath_, PackFileOptions options_)
		: fullFilePath(std::move(fullFilePath_))
		, options(options_) {}

std::unique_ptr<PackFile> PackFile::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	auto extension = std::filesystem::path(path).extension().string();
	if (PackFile::getExtensionRegistry().contains(extension)) {
		return PackFile::getExtensionRegistry()[extension](path, options, callback);
	}
	return nullptr;
}

PackFileType PackFile::getType() const {
	return this->type;
}

std::optional<Entry> PackFile::findEntry(const std::string& filename_, bool includeUnbaked) const {
	auto [dir, name] = ::splitFilenameAndParentDir(filename_);
	if (!this->options.allowUppercaseLettersInFilenames) {
		::toLowerCase(dir);
		::toLowerCase(name);
	}

	if (!dir.empty()) {
		if (dir.length() > 1 && dir.substr(0, 1) == "/") {
			dir = dir.substr(1);
		}
		if (dir.length() > 2 && dir.substr(dir.length() - 1) == "/") {
			dir = dir.substr(0, dir.length() - 2);
		}
	}
	if (this->entries.count(dir)) {
		for (const Entry& entry : this->entries.at(dir)) {
			if (entry.filename == name) {
				return entry;
			}
		}
	}
	if (includeUnbaked && this->unbakedEntries.count(dir)) {
		for (const Entry& unbakedEntry : this->unbakedEntries.at(dir)) {
			if (unbakedEntry.filename == name) {
				return unbakedEntry;
			}
		}
	}
	return std::nullopt;
}

std::optional<std::string> PackFile::readEntryText(const Entry& entry) const {
	auto bytes = this->readEntry(entry);
	if (!bytes) {
		return std::nullopt;
	}
	std::string out;
	for (auto byte : *bytes) {
		if (byte == static_cast<std::byte>(0))
			break;
		out += static_cast<char>(byte);
	}
	return out;
}

bool PackFile::removeEntry(const std::string& filename_) {
	auto [dir, name] = ::splitFilenameAndParentDir(filename_);
	if (!this->options.allowUppercaseLettersInFilenames) {
		::toLowerCase(dir);
		::toLowerCase(name);
	}

	// Check unbaked entries first
	if (this->unbakedEntries.count(dir)) {
		for (auto& [preexistingDir, unbakedEntryVec] : this->unbakedEntries) {
			if (preexistingDir != dir) {
				continue;
			}
			for (auto it = unbakedEntryVec.begin(); it != unbakedEntryVec.end(); ++it) {
				if (it->filename == name) {
					unbakedEntryVec.erase(it);
					return true;
				}
			}
		}
	}

	// If it's not in regular entries either you can't remove it!
	if (!this->entries.count(dir))
		return false;

	for (auto it = this->entries.at(dir).begin(); it != this->entries.at(dir).end(); ++it) {
		if (it->filename == name) {
			this->entries.at(dir).erase(it);
			return true;
		}
	}
	return false;
}

const std::unordered_map<std::string, std::vector<Entry>>& PackFile::getBakedEntries() const {
	return this->entries;
}

const std::unordered_map<std::string, std::vector<Entry>>& PackFile::getUnbakedEntries() const {
	return this->unbakedEntries;
}

std::size_t PackFile::getEntryCount(bool includeUnbaked) const {
	std::size_t count = 0;
	for (const auto& [directory, entries_] : this->entries) {
		count += entries_.size();
	}
	if (includeUnbaked) {
		for (const auto& [directory, entries_] : this->unbakedEntries) {
			count += entries_.size();
		}
	}
	return count;
}

std::string_view PackFile::getFilepath() const {
	return this->fullFilePath;
}

std::string PackFile::getTruncatedFilepath() const {
	return (std::filesystem::path{this->fullFilePath}.parent_path() / this->getTruncatedFilestem()).string();
}

std::string PackFile::getFilename() const {
	return std::filesystem::path{this->fullFilePath}.filename().string();
}

std::string PackFile::getTruncatedFilename() const {
	const std::filesystem::path path{this->fullFilePath};
	return (path.parent_path() / this->getTruncatedFilestem()).string() + path.extension().string();
}

std::string PackFile::getFilestem() const {
	return std::filesystem::path{this->fullFilePath}.stem().string();
}

std::string PackFile::getTruncatedFilestem() const {
	return this->getFilestem();
}

Entry PackFile::createNewEntry() {
	return {};
}

const std::variant<std::string, std::vector<std::byte>>& PackFile::getEntryUnbakedData(const Entry& entry) {
	return entry.unbakedData;
}

void PackFile::setEntryUnbakedData(Entry& entry, const std::variant<std::string, std::vector<std::byte>>& unbakedData) {
	entry.unbakedData = unbakedData;
}

bool PackFile::isEntryUnbakedUsingByteBuffer(const Entry& entry) {
	return entry.unbakedUsingByteBuffer;
}

void PackFile::setEntryUnbakedUsingByteBuffer(Entry& entry, bool unbakedUsingByteBuffer) {
	entry.unbakedUsingByteBuffer = unbakedUsingByteBuffer;
}

std::unordered_map<std::string, PackFile::FactoryFunction>& PackFile::getExtensionRegistry() {
	static std::unordered_map<std::string, PackFile::FactoryFunction> extensionRegistry;
	return extensionRegistry;
}

const PackFile::FactoryFunction& PackFile::registerExtensionForTypeFactory(const std::string& extension, const FactoryFunction& factory) {
	PackFile::getExtensionRegistry()[extension] = factory;
	return factory;
}
