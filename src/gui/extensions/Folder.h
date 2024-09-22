#pragma once

#include <sourcepp/parser/Binary.h>
#include <vpkpp/vpkpp.h>

class Folder : public vpkpp::PackFileReadOnly {
public:
	/// Open a folder
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, const EntryCallback& callback = nullptr);

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
	explicit Folder(const std::string& fullFilePath_);
};
