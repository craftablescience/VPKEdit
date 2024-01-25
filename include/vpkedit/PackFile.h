#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

#include "Entry.h"
#include "Options.h"

namespace vpkedit {

enum class PackFileType {
	UNKNOWN,
	VPK,
};

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

	/// Try to find an entry given the file path
	[[nodiscard]] std::optional<Entry> findEntry(const std::string& filename_, bool includeUnbaked = true) const;

	/// Try to read the entry's data to a bytebuffer
	[[nodiscard]] virtual std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const = 0;

	/// Try to read the entry's data to a string
	[[nodiscard]] std::optional<std::string> readEntryText(const Entry& entry) const;

	/// Add a new entry from a file path - the first parameter is the path in the PackFile, the second is the path on disk
	virtual void addEntry(const std::string& filename_, const std::string& pathToFile, EntryOptions options) = 0;

	/// Add a new entry from a buffer
	virtual void addEntry(const std::string& filename_, std::vector<std::byte>&& buffer, EntryOptions options) = 0;

	/// Add a new entry from a buffer
	virtual void addEntry(const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, EntryOptions options) = 0;

	/// Remove an entry
	bool removeEntry(const std::string& filename_);

	/// If output folder is unspecified, it will overwrite the original
	virtual bool bake(const std::string& outputFolder_ /*= ""*/, const Callback& callback /*= nullptr*/) = 0;

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

protected:
	PackFile(std::string fullFilePath_, PackFileOptions options_);

	[[nodiscard]] static Entry createNewEntry();

	[[nodiscard]] static const std::variant<std::string, std::vector<std::byte>>& getEntryUnbakedData(const Entry& entry);

	static void setEntryUnbakedData(Entry& entry, const std::variant<std::string, std::vector<std::byte>>& unbakedData);

	[[nodiscard]] static bool isEntryUnbakedUsingByteBuffer(const Entry& entry);

	static void setEntryUnbakedUsingByteBuffer(Entry& entry, bool unbakedUsingByteBuffer);

	std::string fullFilePath;

	PackFileType type = PackFileType::UNKNOWN;
	PackFileOptions options;

	std::unordered_map<std::string, std::vector<Entry>> entries;
	std::unordered_map<std::string, std::vector<Entry>> unbakedEntries;
};

} // namespace vpkedit
