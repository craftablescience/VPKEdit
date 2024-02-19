#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Attribute.h"
#include "Entry.h"
#include "Options.h"
#include "PackFileType.h"

namespace vpkedit {

// Executable extensions - mostly just for Godot exports
constexpr std::string_view EXECUTABLE_EXTENSION0 = ".exe";    // - Windows
constexpr std::string_view EXECUTABLE_EXTENSION2 = ".elf";    // - Linux (Generic)
constexpr std::string_view EXECUTABLE_EXTENSION1 = ".bin";    // |- Godot 3 and below (and Generic)
constexpr std::string_view EXECUTABLE_EXTENSION3 = ".x86";    // |- Godot 4 (32-bit)
constexpr std::string_view EXECUTABLE_EXTENSION4 = ".x86_32"; // |- Godot 4 (32-bit)
constexpr std::string_view EXECUTABLE_EXTENSION5 = ".x86_64"; // |- Godot 4 (64-bit)

class PackFile {
public:
	PackFile(const PackFile& other) = delete;
	PackFile& operator=(const PackFile& other) = delete;
	PackFile(PackFile&& other) noexcept = default;
	PackFile& operator=(PackFile&& other) noexcept = default;

	virtual ~PackFile() = default;

	// Accepts the entry parent directory and the entry metadata
	using Callback = std::function<void(const std::string& directory, const Entry& entry)>;

	/// Open a generic pack file. The parser is selected based on the file extension
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	/// Get the file type of the pack file
	[[nodiscard]] PackFileType getType() const;

	/// Get the current options of the pack file
	[[nodiscard]] PackFileOptions getOptions() const;

	/// Verify the checksums of each file, if a file fails the check its filename will be added to the vector.
	/// If there is no checksum ability in the format, it will return an empty vector
	[[nodiscard]] virtual std::vector<std::string> verifyEntryChecksums() const;

	/// Verify the checksum of the entire file, returns true on success
	/// Will return true if there is no checksum ability in the format
	[[nodiscard]] virtual bool verifyFileChecksum() const;

	[[nodiscard]] virtual constexpr bool isCaseSensitive() const noexcept {
		return false;
	}

	/// Try to find an entry given the file path
	[[nodiscard]] std::optional<Entry> findEntry(const std::string& filename_, bool includeUnbaked = true) const;

	/// Try to read the entry's data to a bytebuffer
	[[nodiscard]] virtual std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const = 0;

	/// Try to read the entry's data to a string
	[[nodiscard]] std::optional<std::string> readEntryText(const Entry& entry) const;

	[[nodiscard]] virtual constexpr bool isReadOnly() const noexcept {
		return false;
	}

	/// Add a new entry from a file path - the first parameter is the path in the PackFile, the second is the path on disk
	void addEntry(const std::string& filename_, const std::string& pathToFile, EntryOptions options_);

	/// Add a new entry from a buffer
	void addEntry(const std::string& filename_, std::vector<std::byte>&& buffer, EntryOptions options_);

	/// Add a new entry from a buffer
	void addEntry(const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, EntryOptions options_);

	/// Remove an entry
	virtual bool removeEntry(const std::string& filename_);

	/// If output folder is unspecified, it will overwrite the original
	virtual bool bake(const std::string& outputDir_ /*= ""*/, const Callback& callback /*= nullptr*/) = 0;

	/// Get entries saved to disk
	[[nodiscard]] const std::unordered_map<std::string, std::vector<Entry>>& getBakedEntries() const;

	/// Get entries that have been added but not yet baked
	[[nodiscard]] const std::unordered_map<std::string, std::vector<Entry>>& getUnbakedEntries() const;

	/// Get the number of entries in the pack file
	[[nodiscard]] std::size_t getEntryCount(bool includeUnbaked = true) const;

	/// /home/user/pak01_dir.vpk
	[[nodiscard]] std::string_view getFilepath() const;

	/// /home/user/pak01_dir.vpk -> /home/user/pak01
	[[nodiscard]] std::string getTruncatedFilepath() const;

	/// /home/user/pak01_dir.vpk -> pak01_dir.vpk
	[[nodiscard]] std::string getFilename() const;

	/// /home/user/pak01_dir.vpk -> pak01.vpk
	[[nodiscard]] std::string getTruncatedFilename() const;

	/// /home/user/pak01_dir.vpk -> pak01_dir
	[[nodiscard]] std::string getFilestem() const;

	/// /home/user/pak01_dir.vpk -> pak01
	[[nodiscard]] virtual std::string getTruncatedFilestem() const;

	/// Returns a list of supported entry attributes
	/// Mostly for GUI programs that show entries and their metadata in a table ;)
	[[nodiscard]] virtual std::vector<Attribute> getSupportedEntryAttributes() const;

	[[nodiscard]] virtual explicit operator std::string() const;

	/// Returns a list of supported extensions, e.g. {".vpk", ".bsp"}
	static std::vector<std::string> getSupportedFileTypes();

protected:
	PackFile(std::string fullFilePath_, PackFileOptions options_);

	std::vector<std::string> verifyEntryChecksumsUsingCRC32() const;

	virtual Entry& addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) = 0;

	[[nodiscard]] std::string getBakeOutputDir(const std::string& outputDir) const;

	void mergeUnbakedEntries();

	void setFullFilePath(const std::string& outputDir);

	[[nodiscard]] static Entry createNewEntry();

	[[nodiscard]] static const std::variant<std::string, std::vector<std::byte>>& getEntryUnbakedData(const Entry& entry);

	[[nodiscard]] static bool isEntryUnbakedUsingByteBuffer(const Entry& entry);

	std::string fullFilePath;

	PackFileType type = PackFileType::UNKNOWN;
	PackFileOptions options;

	std::unordered_map<std::string, std::vector<Entry>> entries;
	std::unordered_map<std::string, std::vector<Entry>> unbakedEntries;

	using FactoryFunction = std::function<std::unique_ptr<PackFile>(const std::string& path, PackFileOptions options, const Callback& callback)>;

	static std::unordered_map<std::string, std::vector<FactoryFunction>>& getOpenExtensionRegistry();

	static const FactoryFunction& registerOpenExtensionForTypeFactory(std::string_view extension, const FactoryFunction& factory);
};

class PackFileReadOnly : public PackFile {
public:
	[[nodiscard]] constexpr bool isReadOnly() const noexcept final {
		return true;
	}

	[[nodiscard]] explicit operator std::string() const override;

protected:
	PackFileReadOnly(std::string fullFilePath_, PackFileOptions options_);

	Entry& addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) final;

	bool bake(const std::string& outputDir_ /*= ""*/, const Callback& callback /*= nullptr*/) final;
};

} // namespace vpkedit

#define VPKEDIT_HELPER_CONCAT_INNER(a, b) a ## b
#define VPKEDIT_HELPER_CONCAT(a, b) VPKEDIT_HELPER_CONCAT_INNER(a, b)
#define VPKEDIT_HELPER_UNIQUE_NAME(base) VPKEDIT_HELPER_CONCAT(base, __LINE__)

#define VPKEDIT_REGISTER_PACKFILE_OPEN(extension, function) \
	static inline const FactoryFunction& VPKEDIT_HELPER_UNIQUE_NAME(packFileOpenTypeFactoryFunction) = PackFile::registerOpenExtensionForTypeFactory(extension, function)

#define VPKEDIT_REGISTER_PACKFILE_OPEN_EXECUTABLE(function) \
	static inline const FactoryFunction& VPKEDIT_HELPER_UNIQUE_NAME(packFileOpenExecutable0TypeFactoryFunction) = PackFile::registerOpenExtensionForTypeFactory(vpkedit::EXECUTABLE_EXTENSION0, function); \
	static inline const FactoryFunction& VPKEDIT_HELPER_UNIQUE_NAME(packFileOpenExecutable1TypeFactoryFunction) = PackFile::registerOpenExtensionForTypeFactory(vpkedit::EXECUTABLE_EXTENSION1, function); \
	static inline const FactoryFunction& VPKEDIT_HELPER_UNIQUE_NAME(packFileOpenExecutable2TypeFactoryFunction) = PackFile::registerOpenExtensionForTypeFactory(vpkedit::EXECUTABLE_EXTENSION2, function); \
	static inline const FactoryFunction& VPKEDIT_HELPER_UNIQUE_NAME(packFileOpenExecutable3TypeFactoryFunction) = PackFile::registerOpenExtensionForTypeFactory(vpkedit::EXECUTABLE_EXTENSION3, function); \
	static inline const FactoryFunction& VPKEDIT_HELPER_UNIQUE_NAME(packFileOpenExecutable4TypeFactoryFunction) = PackFile::registerOpenExtensionForTypeFactory(vpkedit::EXECUTABLE_EXTENSION4, function); \
	static inline const FactoryFunction& VPKEDIT_HELPER_UNIQUE_NAME(packFileOpenExecutable5TypeFactoryFunction) = PackFile::registerOpenExtensionForTypeFactory(vpkedit::EXECUTABLE_EXTENSION5, function)
