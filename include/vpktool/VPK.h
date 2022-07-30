/*
 * Contents of this file are part of a C++ port of https://github.com/SteamDatabase/ValvePak, which is licensed under MIT.
 * This port adds VPK1 support and removes CRC checks.
 */

#include <unordered_map>
#include <utility>

#include "FileInputStream.h"

namespace vpktool {

struct VPKEntry {
    /// Gets or sets file name of this entry.
    std::string fileName;
    /// Gets or sets the name of the directory this file is in.
    /// '/' is always used as a dictionary separator in Valve's implementation.
    /// Directory names are also always lower cased in Valve's implementation.
    std::string directoryName;
    /// Gets or sets the file extension.
    /// If the file has no extension, this is an empty string.
    std::string typeName;
    /// CRC32 checksum
    std::uint32_t crc32;
    /// Gets or sets the length in bytes.
    unsigned int length;
    /// Gets or sets the offset in the package.
    unsigned int offset;
    /// Gets or sets which archive this entry is in.
    unsigned short archiveIndex;

    unsigned int getTotalLength() {
        this->totalLength = this->length;
        if (!this->smallData.empty())
            this->totalLength += this->smallData.size();
        return this->totalLength;
    }

    /// Gets or sets the preloaded bytes.
    std::vector<byte> smallData;

    /// Returns the file name and extension.
    [[nodiscard]] std::string getFileName() const {
        if (this->typeName == " ")
            return this->fileName;
        return this->fileName + "." + this->typeName;
    }

    /// Returns the absolute path of the file in the package.
    [[nodiscard]] std::string getFullPath() const {
        if (this->directoryName == " ")
            return this->getFileName();
        return this->directoryName + '/' + this->getFileName();
    }

private:
    /// Gets the length in bytes by adding Length and length of SmallData.
    unsigned int totalLength;
};

struct VPK {
    struct ArchiveMD5SectionEntry {
        /// The CRC32 checksum of this entry.
        std::uint32_t archiveIndex;
        /// The offset in the package.
        std::uint32_t offset;
        /// The length in bytes.
        std::uint32_t length;
        /// The expected Checksum checksum.
        std::vector<byte> checksum;

        ArchiveMD5SectionEntry(std::uint32_t archiveIndex_, std::uint32_t offset_, std::uint32_t length_, std::vector<byte> checksum_)
                : archiveIndex(archiveIndex_)
                , offset(offset_)
                , length(length_)
                , checksum(std::move(checksum_)) {}
    };

    explicit VPK(const std::string& vpkName);

    /// Searches for a given file entry in the file list.
    /// filePath is the full path to the file to find.
    [[nodiscard]] VPKEntry findEntry(const std::string& filePath) const;

    /// Searches for a given file entry in the file list.
    [[nodiscard]] VPKEntry findEntry(const std::string& directory, const std::string& fileName_) const;

    /// Searches for a given file entry in the file list.
    /// directory: Directory to search in
    /// fileName: File name to find, without the extension
    /// extension: File extension, without the leading dot
    [[nodiscard]] VPKEntry findEntry(const std::string& directory, const std::string& fileName_, const std::string& extension) const;

    /// Reads the entry from the VPK package. Returns true on success, false on error.
    /// entry: Package entry
    /// output: Output buffer
    bool readEntry(const VPKEntry& entry, std::vector<byte>& output) const;

    explicit operator bool() const;
    bool operator!() const;

    bool isDirVPK = false;
    unsigned int headerSize = 0;
    /// Gets the filename.
    std::string fileName;
    /// Gets the VPK version.
    unsigned int version = 0;
    /// Gets the size in bytes of the directory tree.
    unsigned int treeSize = 0;
    /// Gets how many bytes of file content are stored in this VPK file (0 in CSGO).
    unsigned int fileDataSectionSize = 0;
    /// Gets the size in bytes of the section containing MD5 checksums for external archive content.
    unsigned int archiveMD5SectionSize = 0;
    /// Gets the size in bytes of the section containing MD5 checksums for content in this file.
    unsigned int otherMD5SectionSize = 0;
    /// Gets the size in bytes of the section containing the public key and signature.
    unsigned int signatureSectionSize = 0;
    /// Gets the MD5 checksum of the file tree.
    std::vector<byte> treeChecksum;
    /// Gets the MD5 checksum of the archive MD5 checksum section entries.
    std::vector<byte> archiveMD5EntriesChecksum;
    /// Gets the MD5 checksum of the complete package until the signature structure.
    std::vector<byte> wholeFileChecksum;
    /// Gets the public key.
    std::vector<byte> publicKey;
    /// Gets the signature.
    std::vector<byte> signature;
    /// Gets the package entries.
    std::unordered_map<std::string, std::vector<VPKEntry>> entries;
    /// Gets the archive MD5 checksum section entries. Also known as cache line hashes.
    std::vector<ArchiveMD5SectionEntry> archiveMD5Entries;
private:
    FileInputStream reader;
    bool isValid = true;
};

} // namespace vpktool
