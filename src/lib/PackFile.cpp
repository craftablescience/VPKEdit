#include <vpkedit/PackFile.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <utility>

#include <vpkedit/detail/CRC32.h>
#include <vpkedit/detail/Misc.h>
#include <vpkedit/format/BSP.h>
#include <vpkedit/format/FPX.h>
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

bool PackFile::verifyFileChecksum() const {
	return true;
}

std::optional<Entry> PackFile::findEntry(const std::string& filename_, bool includeUnbaked) const {
	Entry entry{};
	entry.path = filename_;
	::normalizeSlashes(entry.path);
	if (!this->isCaseSensitive()) {
		::toLowerCase(entry.path);
	}

	if (includeUnbaked) {
		if (auto it = this->unbakedEntries.find(entry); it != this->unbakedEntries.end()) {
			return *it;
		}
	}
	if (auto it = this->entries.find(entry); it != this->entries.end()) {
		return *it;
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

	this->addEntryInternal(entry, filename_, buffer, options_);
}

void PackFile::addEntry(const std::string& filename_, std::vector<std::byte>&& buffer, EntryOptions options_) {
	if (this->isReadOnly()) {
		return;
	}

	Entry entry{};
	entry.unbaked = true;
	entry.unbakedUsingByteBuffer = true;
	entry.unbakedData = std::move(buffer);

	this->addEntryInternal(entry, filename_, buffer, options_);
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

	Entry entry{};
	entry.path = filename_;
	::normalizeSlashes(entry.path);
	if (!this->isCaseSensitive()) {
		::toLowerCase(entry.path);
	}

	if (auto it = this->unbakedEntries.find(entry); it != this->unbakedEntries.end()) {
		this->unbakedEntries.erase(it);
		return true;
	}
	if (auto it = this->entries.find(entry); it != this->entries.end()) {
		this->entries.erase(it);
		return true;
	}
	return false;
}

const std::unordered_set<Entry>& PackFile::getBakedEntries() const {
	return this->entries;
}

const std::unordered_set<Entry>& PackFile::getUnbakedEntries() const {
	return this->unbakedEntries;
}

std::size_t PackFile::getEntryCount(bool includeUnbaked) const {
	return this->entries.size() + (includeUnbaked ? this->unbakedEntries.size() : 0);
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
	for (const auto& entry : this->entries) {
		if (!entry.crc32) {
			continue;
		}
		if (auto data = this->readEntry(entry); !data || ::computeCRC32(*data) != entry.crc32) {
			out.push_back(entry.path);
		}
	}
	for (const auto& entry : this->unbakedEntries) {
		if (!entry.crc32) {
			continue;
		}
		if (auto data = this->readEntry(entry); !data || ::computeCRC32(*data) != entry.crc32) {
			out.push_back(entry.path);
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

std::optional<std::vector<std::byte>> PackFile::readUnbakedEntry(const Entry& unbakedEntry) const {
	if (auto it = this->unbakedEntries.find(unbakedEntry); it != this->unbakedEntries.end()) {
		// Get the stored data
		std::vector<std::byte> unbakedData;
		if (isEntryUnbakedUsingByteBuffer(unbakedEntry)) {
			unbakedData = std::get<std::vector<std::byte>>(getEntryUnbakedData(unbakedEntry));
		} else {
			unbakedData = ::readFileData(std::get<std::string>(getEntryUnbakedData(unbakedEntry)));
		}
		return unbakedData;
	}
	return std::nullopt;
}

void PackFile::mergeUnbakedEntries() {
	for (auto entry : this->unbakedEntries) {
		this->unbakedEntries.erase(entry);

		entry.unbaked = false;
		entry.unbakedUsingByteBuffer = false;
		entry.unbakedData = "";

		this->entries.insert(std::move(entry));
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

void PackFileReadOnly::addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) {
	// Stubbed
}

bool PackFileReadOnly::bake(const std::string& outputDir_ /*= ""*/, const Callback& callback /*= nullptr*/) {
	return false; // Stubbed
}
