#include <vpkedit/PackFile.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <utility>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/Misc.h>
#include <vpkedit/format/BSP.h>
#include <vpkedit/format/GCF.h>
#include <vpkedit/format/GMA.h>
#include <vpkedit/format/GRP.h>
#include <vpkedit/format/PAK.h>
#include <vpkedit/format/PCK.h>
#include <vpkedit/format/VPK.h>
#include <vpkedit/format/ZIP.h>

using namespace vpkedit;
using namespace vpkedit::detail;

PackFile::PackFile(std::string fullFilePath_, PackFileOptions options_)
		: fullFilePath(std::move(fullFilePath_))
		, options(options_) {}

std::unique_ptr<PackFile> PackFile::open(const std::string& path, PackFileOptions options, const Callback& callback) {
	auto extension = std::filesystem::path(path).extension().string();
	::toLowerCase(extension);
	const auto& registry = PackFile::getOpenExtensionRegistry();
	if (registry.contains(extension)) {
		for (const auto& func : registry.at(extension)) {
			if (auto packFile = func(path, options, callback)) {
				return packFile;
			}
		}
	}
	return nullptr;
}

PackFileType PackFile::getType() const {
	return this->type;
}

PackFileOptions PackFile::getOptions() const {
	return this->options;
}

std::vector<std::string> PackFile::verifyEntryChecksums() const {
	return {};
}

bool PackFile::hasFileChecksum() const {
	return false;
}

bool PackFile::verifyFileChecksum() const {
	return true;
}

bool PackFile::hasFileSignature() const {
	return false;
}

bool PackFile::verifyFileSignature() const {
	return true;
}

std::optional<Entry> PackFile::findEntry(const std::string& filename_, bool includeUnbaked) const {
	auto filename = filename_;
	::normalizeSlashes(filename);
	if (!this->isCaseSensitive()) {
		::toLowerCase(filename);
	}
	auto [dir, name] = ::splitFilenameAndParentDir(filename);

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
	if (this->isReadOnly()) {
		return;
	}

	auto buffer = ::readFileData(pathToFile, 0);

	Entry entry{};
	entry.unbaked = true;
	entry.unbakedUsingByteBuffer = false;
	entry.unbakedData = pathToFile;

	Entry& finalEntry = this->addEntryInternal(entry, filename_, buffer, options_);
	finalEntry.unbakedData = pathToFile;
}

void PackFile::addEntry(const std::string& filename_, std::vector<std::byte>&& buffer, EntryOptions options_) {
	if (this->isReadOnly()) {
		return;
	}

	Entry entry{};
	entry.unbaked = true;
	entry.unbakedUsingByteBuffer = true;

	Entry& finalEntry = this->addEntryInternal(entry, filename_, buffer, options_);
	finalEntry.unbakedData = std::move(buffer);
}

void PackFile::addEntry(const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, EntryOptions options_) {
	std::vector<std::byte> data;
	if (buffer && bufferLen > 0) {
		data.resize(bufferLen);
		std::memcpy(data.data(), buffer, bufferLen);
	}
	this->addEntry(filename_, std::move(data), options_);
}

bool PackFile::removeEntry(const std::string& filename_) {
	if (this->isReadOnly()) {
		return false;
	}

	auto filename = filename_;
	if (!this->isCaseSensitive()) {
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

std::optional<std::vector<std::byte>> PackFile::readVirtualEntry(const VirtualEntry& entry) const {
	return std::nullopt;
}

bool PackFile::overwriteVirtualEntry(const VirtualEntry& entry, const std::string& pathToFile) {
	return this->overwriteVirtualEntry(entry, ::readFileData(pathToFile));
}

bool PackFile::overwriteVirtualEntry(const VirtualEntry& entry, const std::vector<std::byte>& data) {
	return false;
}

std::vector<VirtualEntry> PackFile::getVirtualEntries() const {
	return {};
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
	return this->getTruncatedFilestem() + path.extension().string();
}

std::string PackFile::getFilestem() const {
	return std::filesystem::path{this->fullFilePath}.stem().string();
}

std::string PackFile::getTruncatedFilestem() const {
	return this->getFilestem();
}

std::vector<Attribute> PackFile::getSupportedEntryAttributes() const {
	return {};
}

PackFile::operator std::string() const {
	return this->getFilename();
}

std::vector<std::string> PackFile::getSupportedFileTypes() {
	std::vector<std::string> out;
	for (const auto& [extension, factoryFunctions] : PackFile::getOpenExtensionRegistry()) {
		out.push_back(extension);
	}
	std::sort(out.begin(), out.end());
	return out;
}

std::vector<std::string> PackFile::verifyEntryChecksumsUsingCRC32() const {
	std::vector<std::string> out;
	for (const auto& [dir, entryList] : this->entries) {
		for (const auto& entry : entryList) {
			if (!entry.crc32) {
				continue;
			}
			if (auto data = this->readEntry(entry); !data || ::computeCRC32(*data) != entry.crc32) {
				out.push_back(entry.path);
			}
		}
	}
	for (const auto& [dir, entryList] : this->unbakedEntries) {
		for (const auto& entry : entryList) {
			if (!entry.crc32) {
				continue;
			}
			if (auto data = this->readEntry(entry); !data || ::computeCRC32(*data) != entry.crc32) {
				out.push_back(entry.path);
			}
		}
	}
	return out;
}

std::string PackFile::getBakeOutputDir(const std::string& outputDir) const {
	std::string out = outputDir;
	if (!out.empty()) {
		::normalizeSlashes(out, false);
	} else {
		out = this->fullFilePath;
		auto lastSlash = out.rfind('/');
		if (lastSlash != std::string::npos) {
			out = this->getFilepath().substr(0, lastSlash);
		} else {
			out = ".";
		}
	}
	return out;
}

void PackFile::mergeUnbakedEntries() {
	for (auto& [dir, unbakedEntriesAndData] : this->unbakedEntries) {
		for (Entry& unbakedEntry : unbakedEntriesAndData) {
			if (!this->entries.contains(dir)) {
				this->entries[dir] = {};
			}

			unbakedEntry.unbaked = false;

			// Clear any data that might be stored in it
			unbakedEntry.unbakedUsingByteBuffer = false;
			unbakedEntry.unbakedData = "";

			this->entries.at(dir).push_back(unbakedEntry);
		}
	}
	this->unbakedEntries.clear();
}

void PackFile::setFullFilePath(const std::string& outputDir) {
	// Assumes PackFile::getBakeOutputDir is the input for outputDir
	this->fullFilePath = outputDir + '/' + this->getFilename();
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

std::unordered_map<std::string, std::vector<PackFile::FactoryFunction>>& PackFile::getOpenExtensionRegistry() {
	static std::unordered_map<std::string, std::vector<PackFile::FactoryFunction>> extensionRegistry;
	return extensionRegistry;
}

const PackFile::FactoryFunction& PackFile::registerOpenExtensionForTypeFactory(std::string_view extension, const FactoryFunction& factory) {
	std::string extensionStr{extension};
	auto& registry = PackFile::getOpenExtensionRegistry();
	if (!registry.contains(extensionStr)) {
		registry[extensionStr] = {};
	}
	registry[extensionStr].push_back(factory);
	return factory;
}

PackFileReadOnly::PackFileReadOnly(std::string fullFilePath_, PackFileOptions options_)
		: PackFile(std::move(fullFilePath_), options_) {}

PackFileReadOnly::operator std::string() const {
	return PackFile::operator std::string() + " (Read-Only)";
}

Entry& PackFileReadOnly::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	return entry; // Stubbed
}

bool PackFileReadOnly::bake(const std::string& outputDir_ /*= ""*/, const Callback& callback /*= nullptr*/) {
	return false; // Stubbed
}
