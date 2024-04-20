#pragma once

#include <array>
#include <cstdint>
#include <tuple>

#include "../PackFile.h"

namespace vpkedit {

constexpr std::uint32_t VPK_SIGNATURE = 0x55aa1234;
constexpr std::uint32_t VPK_DIR_INDEX = 0x7fff;
constexpr std::uint16_t VPK_ENTRY_TERM = 0xffff;
constexpr std::string_view VPK_DIR_SUFFIX = "_dir";
constexpr std::string_view VPK_EXTENSION = ".vpk";

constexpr std::string_view VPK_KEYPAIR_PUBLIC_KEY_TEMPLATE = "public_key\n{\n\ttype \"rsa\"\n\trsa_public_key \"%s\"\n}\n";
constexpr std::string_view VPK_KEYPAIR_PRIVATE_KEY_TEMPLATE = "private_key\n{\n\ttype \"rsa\"\n\trsa_private_key \"%s\"\n\tprivate_key_encrypted 0\n\tpublic_key\n\t{\n\t\ttype \"rsa\"\n\t\trsa_public_key \"%s\"\n\t}\n}\n";

class VPK : public PackFile {
protected:
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

	struct FreedChunk {
		std::uint64_t offset;
		std::uint64_t length;
		std::uint16_t archiveIndex;
	};

public:
	// Accepts the full entry path (parent directory + filename), returns saveToDir and preloadBytes
	using EntryCreationCallback = std::function<std::tuple<bool, std::uint32_t>(const std::string& fullEntryPath)>;

	/// Create a new directory VPK file - must end in "_dir.vpk"! This is not enforced but STRONGLY recommended
	[[nodiscard]] static std::unique_ptr<PackFile> createEmpty(const std::string& path, PackFileOptions options = {});

	/// Create a new directory VPK file from a directory, the contents of the directory will be present in the root VPK directory. (See above comment)
	[[nodiscard]] static std::unique_ptr<PackFile> createFromDirectory(const std::string& vpkPath, const std::string& contentPath, bool saveToDir = false, PackFileOptions options = {}, const Callback& bakeCallback = nullptr);

	/// Create a new directory VPK file from a directory, the contents of the directory will be present in the root VPK directory. Each entry's properties is determined by a callback. (See above comment)
	[[nodiscard]] static std::unique_ptr<PackFile> createFromDirectoryProcedural(const std::string& vpkPath, const std::string& contentPath, const EntryCreationCallback& creationCallback, PackFileOptions options = {}, const Callback& bakeCallback = nullptr);

	/// Open a directory VPK file
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] std::vector<std::string> verifyEntryChecksums() const override;

	[[nodiscard]] bool verifyFileChecksum() const override;

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const override;

	bool removeEntry(const std::string& filename_) override;

	bool bake(const std::string& outputDir_ /*= ""*/, const Callback& callback /*= nullptr*/) override;

	[[nodiscard]] std::string getTruncatedFilestem() const override;

	[[nodiscard]] std::vector<Attribute> getSupportedEntryAttributes() const override;

	[[nodiscard]] explicit operator std::string() const override;

	/// Generate keypair files, which can be used to sign a VPK
	/// Input is a truncated file path, e.g. "/x/y/z/key" or just "key" for the CWD
	/// It will append ".publickey.vdf" and ".privatekey.vdf" to the input and save those files
	static void generateKeyPairFiles(const std::string& name);

	/// Sign the VPK with the given keypair. Run this after VPK::bake if modifying the VPK file contents
	bool sign(const std::vector<std::byte>& privateKey, const std::vector<std::byte>& publicKey);

	/// Returns 1 for v1, 2 for v2
	[[nodiscard]] std::uint32_t getVersion() const;

	/// Change the version of the VPK. Valid values are 1 and 2
	void setVersion(std::uint32_t version);

protected:
	VPK(const std::string& fullFilePath_, PackFileOptions options_);

	[[nodiscard]] static std::unique_ptr<PackFile> openInternal(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	Entry& addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) override;

	[[nodiscard]] std::uint32_t getHeaderLength() const;

	int numArchives = -1;
	std::uint32_t currentlyFilledChunkSize = 0;

	std::vector<FreedChunk> freedChunks;

	Header1 header1{}; // Present in all VPK versions
	Header2 header2{}; // Present in VPK v2
	Footer2 footer2{}; // Present in VPK v2

	std::vector<MD5Entry> md5Entries;

private:
	VPKEDIT_REGISTER_PACKFILE_OPEN(VPK_EXTENSION, &VPK::open);
};

} // namespace vpkedit
