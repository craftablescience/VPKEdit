#pragma once

#include <cstdint>

#include "../PackFile.h"

namespace vpkedit {

constexpr std::int32_t PCK_SIGNATURE = 0x43504447;
constexpr std::string_view PCK_PATH_PREFIX = "res://";
constexpr std::string_view PCK_EXTENSION = ".pck";

class PCK : public PackFileReadOnly {
protected:
	struct Header {
		std::uint32_t packVersion;
		std::uint32_t godotVersionMajor;
		std::uint32_t godotVersionMinor;
		std::uint32_t godotVersionPatch;
		std::uint32_t flags; // packVersion 2+
	};

public:
	/// Open a PCK file (potentially embedded in an executable)
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

	[[nodiscard]] constexpr bool isCaseSensitive() const noexcept override {
		return true;
	}

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const Entry& entry) const override;

	[[nodiscard]] std::vector<Attribute> getSupportedEntryAttributes() const override;

	[[nodiscard]] explicit operator std::string() const override;

protected:
	PCK(const std::string& fullFilePath_, PackFileOptions options_);

	Header header{};

	bool embedded = false;
	std::size_t startOffset = 0;
	std::size_t entryContentsOffset = 0;

private:
	VPKEDIT_REGISTER_PACKFILE_OPEN(PCK_EXTENSION, &PCK::open);
	VPKEDIT_REGISTER_PACKFILE_OPEN_EXECUTABLE(&PCK::open);
};

} // namespace vpkedit
