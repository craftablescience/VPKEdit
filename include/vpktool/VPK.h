/*
 * Contents of this file are part of a C++ port of https://github.com/SteamDatabase/ValvePak, which is licensed under MIT.
 * This port adds VPK1 support and removes CRC checks.
 */

#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "InputStream.h"

namespace vpktool {

struct VPKEntry {
    /// File name of this entry (e.g. "cable.vmt")
    std::string filename;
    /// CRC32 checksum
    std::uint32_t crc32;
    /// Length in bytes
    std::uint32_t length;
    /// Offset in the VPK
    std::uint32_t offset;
    /// Which VPK this entry is in
    std::uint16_t archiveIndex;
    /// Preloaded data
    std::vector<std::byte> preloadedData;
};

struct VPK {
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

    struct MD5Entry {
        /// The CRC32 checksum of this entry.
        std::uint32_t archiveIndex;
        /// The offset in the package.
        std::uint32_t offset;
        /// The length in bytes.
        std::uint32_t length;
        /// The expected Checksum checksum.
        std::byte checksum[16];
    };
#pragma pack(pop)

    [[nodiscard]] static std::optional<VPK> open(const std::string& path);

    [[nodiscard]] std::optional<VPKEntry> findEntry(const std::string& filename_) const;
    [[nodiscard]] std::optional<VPKEntry> findEntry(const std::string& directory, const std::string& filename_) const;

    [[nodiscard]] std::vector<std::byte> readBinaryEntry(const VPKEntry& entry) const;
    [[nodiscard]] std::string readTextEntry(const VPKEntry& entry) const;

    [[nodiscard]] const std::unordered_map<std::string, std::vector<VPKEntry>>& getEntries() const {
        return this->entries;
    }

    [[nodiscard]] std::uint32_t getHeaderLength() const {
        if (!this->header2.fileDataSectionSize) {
            return sizeof(Header1);
        }
        return sizeof(Header1) + sizeof(Header2);
    }

protected:
    std::string filename;

    Header1 header1{};
    Header2 header2{};
    std::array<std::byte, 16> treeChecksum{};
    std::array<std::byte, 16> md5EntriesChecksum{};
    std::array<std::byte, 16> wholeFileChecksum{};
    std::vector<std::byte> publicKey;
    std::vector<std::byte> signature;

    std::unordered_map<std::string, std::vector<VPKEntry>> entries;
    std::vector<MD5Entry> md5Entries;

    InputStream reader;

private:
    VPK(InputStream&& reader_, std::string filename_);

    [[nodiscard]] static bool open(VPK& vpk);
};

} // namespace vpktool
