#pragma once

#include <vpkpp/vpkpp.h>

class Folder : public vpkpp::PackFileReadOnly {
public:
	/// Open a folder
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, const EntryCallback& callback = nullptr);

	static constexpr inline std::string_view GUID = "7EC88CB090BC496DB5C68149F0A7E254";

	[[nodiscard]] constexpr std::string_view getGUID() const override {
		return Folder::GUID;
	}

	[[nodiscard]] constexpr bool isCaseSensitive() const noexcept override {
#ifdef _WIN32
		return false;
#else
		return true;
#endif
	}

	[[nodiscard]] std::optional<std::vector<std::byte>> readEntry(const std::string& path_) const override;

	[[nodiscard]] vpkpp::Attribute getSupportedEntryAttributes() const override;

protected:
	using PackFileReadOnly::PackFileReadOnly;
};
