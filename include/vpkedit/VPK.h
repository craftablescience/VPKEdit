#pragma once

#include <array>
#include <optional>
#include <unordered_map>
#include <utility>
#include <string_view>
#include <tuple>

#include <vpkedit/detail/FileStream.h>
#include <vpkedit/PackFile.h>

namespace vpkedit {

constexpr std::uint32_t VPK_ID = 0x55aa1234;
constexpr std::uint32_t VPK_DIR_INDEX = 0x7fff;
constexpr std::uint16_t VPK_ENTRY_TERM = 0xffff;

class VPK : public PackFile {
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
		/// The archive index of the file
        std::uint32_t archiveIndex;
        /// The offset in the archive
        std::uint32_t offset;
        /// The length in bytes
        std::uint32_t length;
	    /// The CRC32 checksum of this entry
        std::array<std::byte, 16> checksum;
    };
#pragma pack(pop)

public:
	// Accepts the full entry path (parent directory + filename), returns saveToDir and preloadBytes
	using EntryCreationCallback = std::function<std::tuple<bool, std::uint32_t>(const std::string& fullEntryPath)>;

    /// Create a new directory VPK file - must end in "_dir.vpk"! This is not enforced but STRONGLY recommended
    [[nodiscard]] static std::unique_ptr<PackFile> createEmpty(const std::string& path, PackFileOptions options = {});

    /// Create a new directory VPK file from a directory, the contents of the directory will be present in the root VPK directory. (See above comment)
    [[nodiscard]] static std::unique_ptr<PackFile> createFromDirectory(const std::string& vpkPath, const std::string& contentPath, bool saveToDir = true, PackFileOptions options = {}, const Callback& bakeCallback = nullptr);

	/// Create a new directory VPK file from a directory, the contents of the directory will be present in the root VPK directory. Each entry's properties is determined by a callback. (See above comment)
	[[nodiscard]] static std::unique_ptr<PackFile> createFromDirectoryProcedural(const std::string& vpkPath, const std::string& contentPath, const EntryCreationCallback& creationCallback, PackFileOptions options = {}, const Callback& bakeCallback = nullptr);

    /// Open a directory VPK file
    [[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

    [[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const final;

    void addEntry(const std::string& filename_, const std::string& pathToFile, EntryOptions options) final;

    void addEntry(const std::string& filename_, std::vector<std::byte>&& buffer, EntryOptions options) final;

    void addEntry(const std::string& filename_, const std::byte* buffer, std::uint64_t bufferLen, EntryOptions options) final;

    bool bake(const std::string& outputFolder_ /*= ""*/, const Callback& callback /*= nullptr*/) final;

	[[nodiscard]] std::string getTruncatedFilestem() const final;

    /// Returns 1 for v1, 2 for v2
    [[nodiscard]] std::uint32_t getVersion() const;

	/// Change the version of the VPK. Valid values are 1 and 2
    void setVersion(std::uint32_t version);

protected:
    VPK(const std::string& fullFilePath_, PackFileOptions options_);

	detail::FileStream reader;

	int numArchives = -1;
	std::uint32_t currentlyFilledChunkSize = 0;

    Header1 header1{}; // Present in all VPK versions
    Header2 header2{}; // Present in VPK v2
    Footer2 footer2{}; // Present in VPK v2

    std::vector<MD5Entry> md5Entries;

private:
	[[nodiscard]] std::uint32_t getHeaderLength() const;

	VPKEDIT_REGISTER_PACKFILE_EXTENSION(".vpk", [](const std::string& path, PackFileOptions options, const Callback& callback) {
		auto vpk = VPK::open(path, options, callback);
		if (!vpk && path.length() > 8) {
			// If it just tried to load a numbered archive, let's try to load the directory VPK
			vpk = VPK::open(path.substr(0, path.length() - 8) + "_dir.vpk", options, callback);
		}
		return vpk;
	});
};

} // namespace vpkedit
