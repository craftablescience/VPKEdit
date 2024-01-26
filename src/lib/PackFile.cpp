#include <vpkedit/PackFile.h>

#include <filesystem>
#include <utility>

#include <vpkedit/detail/Misc.h>
#include <vpkedit/BSP.h>
#include <vpkedit/VPK.h>
#include <vpkedit/ZIP.h>

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
	auto filename = filename_;
	::normalizeSlashes(filename);
	if (!this->options.allowUppercaseLettersInFilenames) {
		::toLowerCase(filename);
	}
	auto [dir, name] = ::splitFilenameAndParentDir(filename);

	if (!dir.empty()) {
		if (dir.length() > 1 && dir.substr(0, 1) == "/") {
			dir = dir.substr(1);
		}
		if (dir.length() > 2 && dir.substr(dir.length() - 1) == "/") {
			dir = dir.substr(0, dir.length() - 2);
		}
	}
	if (this->entries.contains(dir)) {
		for (const Entry& entry : this->entries.at(dir)) {
			if (entry.path == filename) {
				return entry;
			}
		}
	}
	if (includeUnbaked && this->unbakedEntries.contains(dir)) {
		for (const Entry& unbakedEntry : this->unbakedEntries.at(dir)) {
			if (unbakedEntry.path == filename) {
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

void PackFile::addEntry(const std::string& filename_, const std::string& pathToFile, EntryOptions options_) {
	auto buffer = ::readFileData(pathToFile, 0);

	Entry entry{};
	entry.unbaked = true;
	entry.unbakedUsingByteBuffer = false;
	entry.unbakedData = pathToFile;

	Entry& finalEntry = this->addEntryInternal(entry, filename_, buffer, options_);
	finalEntry.unbakedData = pathToFile;
}

void PackFile::addEntry(const std::string& filename_, std::vector<std::byte>&& buffer, EntryOptions options_) {
	Entry entry{};
	entry.unbaked = true;
	entry.unbakedUsingByteBuffer = true;

	Entry& finalEntry = this->addEntryInternal(entry, filename_, buffer, options_);
	finalEntry.unbakedData = std::move(buffer);
}

void PackFile::addEntry(const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, EntryOptions options_) {
	std::vector<std::byte> data;
	data.resize(bufferLen);
	std::memcpy(data.data(), buffer, bufferLen);
	this->addEntry(filename_, std::move(data), options_);
}

bool PackFile::removeEntry(const std::string& filename_) {
	auto filename = filename_;
	if (!this->options.allowUppercaseLettersInFilenames) {
		::toLowerCase(filename);
	}
	auto [dir, name] = ::splitFilenameAndParentDir(filename);

	// Check unbaked entries first
	if (this->unbakedEntries.contains(dir)) {
		for (auto& [preexistingDir, unbakedEntryVec] : this->unbakedEntries) {
			if (preexistingDir != dir) {
				continue;
			}
			for (auto it = unbakedEntryVec.begin(); it != unbakedEntryVec.end(); ++it) {
				if (it->path == filename) {
					unbakedEntryVec.erase(it);
					return true;
				}
			}
		}
	}

	// If it's not in regular entries either you can't remove it!
	if (!this->entries.contains(dir))
		return false;

	for (auto it = this->entries.at(dir).begin(); it != this->entries.at(dir).end(); ++it) {
		if (it->path == filename) {
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

std::vector<std::string> PackFile::getSupportedFileTypes() {
	std::vector<std::string> out;
	for (const auto& [extension, factoryFunction] : PackFile::getExtensionRegistry()) {
		out.push_back(extension);
	}
	return out;
}

Entry PackFile::createNewEntry() {
	return {};
}

const std::variant<std::string, std::vector<std::byte>>& PackFile::getEntryUnbakedData(const Entry& entry) {
	return entry.unbakedData;
}

bool PackFile::isEntryUnbakedUsingByteBuffer(const Entry& entry) {
	return entry.unbakedUsingByteBuffer;
}

std::unordered_map<std::string, PackFile::FactoryFunction>& PackFile::getExtensionRegistry() {
	static std::unordered_map<std::string, PackFile::FactoryFunction> extensionRegistry;
	return extensionRegistry;
}

const PackFile::FactoryFunction& PackFile::registerExtensionForTypeFactory(const std::string& extension, const FactoryFunction& factory) {
	PackFile::getExtensionRegistry()[extension] = factory;
	return factory;
}
