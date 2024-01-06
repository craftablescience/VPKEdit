#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <utility>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <vpkedit/detail/FileStream.h>

namespace vpkedit {

constexpr std::uint32_t VPK_ID = 0x55aa1234;
constexpr std::uint32_t VPK_DIR_INDEX = 0x7fff;
constexpr std::uint16_t VPK_ENTRY_TERM = 0xffff;
constexpr int VPK_MAX_PRELOAD_BYTES = 1024;

struct VPKEntry {
private:
	friend class VPK;

public:
    /// File name of this entry (e.g. "cable.vmt")
    std::string filename;
    /// CRC32 checksum
    std::uint32_t crc32 = 0;
    /// Length in bytes
    std::uint32_t length = 0;
    /// Offset in the VPK
    std::uint32_t offset = 0;
    /// Which VPK this entry is in
    std::uint16_t archiveIndex = 0;
    /// Preloaded data
    std::vector<std::byte> preloadedData;
    /// Used to check if entry is saved in the loaded VPK
    bool unbaked = false;

	/// Returns the file stem (e.g. "cable.vmt" -> "cable")
	[[nodiscard]] std::string getStem() const {
		return std::filesystem::path{this->filename}.stem().string();
	}

	/// Returns the file extension without a period (e.g. "cable.vmt" -> "vmt")
	[[nodiscard]] std::string getExtension() const {
		auto ext = std::filesystem::path{this->filename}.extension().string();
		if (!ext.empty() && ext.at(0) == '.') {
			ext = ext.substr(1);
		}
		return ext;
	}

private:
	/// The data attached to the unbaked entry, or the path to the file containing the unbaked entry's data
	std::variant<std::string, std::vector<std::byte>> unbakedData;
	/// Which one?
	bool unbakedUsingByteBuffer = false;

    VPKEntry() = default;
};

class VPK {
#pragma pack(push, 1)
    struct Header1 {
        std::uint32_t signature;
        std::uint32_t version;
        std::uint32_t treeSize;
    };

    struct Header2 {
        std::uint32_t fileDataSectionSize;
        std::uint32_t archiveMD5SectionSize;
        std::uint32_t otherMD5SectionSize;
        std::uint32_t signatureSectionSize;
    };

    struct Footer2 {
        std::array<std::byte, 16> treeChecksum{};
        std::array<std::byte, 16> md5EntriesChecksum{};
        std::array<std::byte, 16> wholeFileChecksum{};
        std::vector<std::byte> publicKey{};
        std::vector<std::byte> signature{};
    };

    struct MD5Entry {
        /// The CRC32 checksum of this entry.
        std::uint32_t archiveIndex;
        /// The offset in the package.
        std::uint32_t offset;
        /// The length in bytes.
        std::uint32_t length;
        /// The expected Checksum checksum.
        std::array<std::byte, 16> checksum;
    };
#pragma pack(pop)

public:
    VPK(const VPK& other) = delete;
    VPK& operator=(const VPK& other) = delete;
    VPK(VPK&& other) noexcept = default;
    VPK& operator=(VPK&& other) noexcept = default;

    /// Create a new directory VPK file - must end in "_dir.vpk"! This is not enforced but STRONGLY recommended
    [[nodiscard]] static VPK createEmpty(const std::string& path, std::uint32_t version = 2);

    /// Create a new directory VPK file from a directory (see above comment)
    [[nodiscard]] static VPK createFromDirectory(const std::string& vpkPath, const std::string& directoryPath, std::uint32_t version = 2);

    /// Open a directory VPK file
    [[nodiscard]] static std::optional<VPK> open(const std::string& path);

    /// Open a directory VPK from memory
    /// Note that any content not stored in the directory VPK will fail to load!
    /// Also baking new entries will fail
    [[nodiscard]] static std::optional<VPK> open(std::byte* buffer, std::uint64_t bufferLen);

	/// Try to find an entry within the VPK given the file path
    [[nodiscard]] std::optional<VPKEntry> findEntry(const std::string& filename_, bool includeUnbaked = true) const;

	/// Try to read an entry within the VPK (returns binary)
    [[nodiscard]] std::optional<std::vector<std::byte>> readBinaryEntry(const VPKEntry& entry) const;

	/// Try to read an entry within the VPK (returns text)
    [[nodiscard]] std::optional<std::string> readTextEntry(const VPKEntry& entry) const;

    void addEntry(const std::string& filename_, const std::string& pathToFile, bool saveToDir = true, int preloadBytes = 0);

    void addBinaryEntry(const std::string& filename_, std::vector<std::byte>&& buffer, bool saveToDir = true, int preloadBytes = 0);

    void addBinaryEntry(const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, bool saveToDir = true, int preloadBytes = 0);

    void addTextEntry(const std::string& filename_, const std::string& text, bool saveToDir = true, int preloadBytes = VPK_MAX_PRELOAD_BYTES);

    bool removeEntry(const std::string& filename_);

    /// If output folder is unspecified, it will overwrite the original
    bool bake(const std::string& outputFolder_ = "");

    /// Returns 1 for v1, 2 for v2
    [[nodiscard]] std::uint32_t getVersion() const;

	/// Change the version of the VPK. Valid values are 1 and 2
    void setVersion(std::uint32_t version);

    [[nodiscard]] const std::unordered_map<std::string, std::vector<VPKEntry>>& getBakedEntries() const {
        return this->entries;
    }

    [[nodiscard]] const std::unordered_map<std::string, std::vector<VPKEntry>>& getUnbakedEntries() const {
        return this->unbakedEntries;
    }

    [[nodiscard]] std::uint32_t getHeaderLength() const {
        if (this->header1.version < 2) {
            return sizeof(Header1);
        }
        return sizeof(Header1) + sizeof(Header2);
    }

    /// pak01_dir.vpk -> pak01
    [[nodiscard]] std::string_view getPrettyFilename() const {
        // Find the last occurrence of the slash character
        if (std::size_t lastSlashIndex = this->filename.find_last_of('/'); lastSlashIndex != std::string::npos) {
            return {this->filename.data() + lastSlashIndex + 1, this->filename.length() - lastSlashIndex - 1};
        }
        return this->filename; // not much else to do, should never happen
    }

    /// pak01_dir.vpk -> pak01_dir
    [[nodiscard]] std::string getRealFilename() const;

protected:
    VPK(detail::FileStream&& reader_, std::string fullPath_, std::string filename_);

    std::string fullPath;
    std::string filename;
    int numArchives = -1;

    Header1 header1{}; // Present in all VPK versions
    Header2 header2{}; // Present in VPK v2
    Footer2 footer2{}; // Present in VPK v2

    std::unordered_map<std::string, std::vector<VPKEntry>> entries;
    std::unordered_map<std::string, std::vector<VPKEntry>> unbakedEntries;
    std::vector<MD5Entry> md5Entries;

    detail::FileStream reader;

private:
    [[nodiscard]] static bool open(VPK& vpk);
};

} // namespace vpkedit
