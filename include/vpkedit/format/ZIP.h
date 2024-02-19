#pragma once

#include <string_view>

#include "../PackFile.h"

namespace vpkedit {

constexpr std::string_view ZIP_EXTENSION = ".zip";

class ZIP : public PackFile {
public:
	~ZIP() override;

	/// Open a ZIP file
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] std::vector<std::string> verifyEntryChecksums() const override;

	[[nodiscard]] constexpr bool isCaseSensitive() const noexcept override {
		return true;
	}

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const override;

	bool bake(const std::string& outputDir_ /*= ""*/, const Callback& callback /*= nullptr*/) override;

	[[nodiscard]] std::vector<Attribute> getSupportedEntryAttributes() const override;

#ifdef VPKEDIT_ZIP_COMPRESSION
	[[nodiscard]] std::uint16_t getCompressionMethod() const;

	void setCompressionMethod(std::uint16_t compressionMethod);
#endif

protected:
	ZIP(const std::string& fullFilePath_, PackFileOptions options_);

	Entry& addEntryInternal(Entry& entry, const std::string& filename_, std::vector<std::byte>& buffer, EntryOptions options_) override;

	bool bakeTempZip(const std::string& writeZipPath, const Callback& callback);

	bool openZIP(std::string_view path);

	void closeZIP();

	static const std::string TEMP_ZIP_PATH;

	void* streamHandle = nullptr;
	bool streamOpen = false;

	void* zipHandle = nullptr;
	bool zipOpen = false;

private:
	VPKEDIT_REGISTER_PACKFILE_OPEN(ZIP_EXTENSION, &ZIP::open);
};

} // namespace vpkedit
