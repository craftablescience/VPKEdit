#pragma once

#include "VPK.h"

namespace vpkedit {

constexpr std::uint32_t FPX_SIGNATURE = 0x3241ff33;
constexpr std::string_view FPX_DIR_SUFFIX = "_fdr";
constexpr std::string_view FPX_EXTENSION = ".fpx";

class FPX : public VPK {
public:
	/// Open a directory VPK file
	[[nodiscard]] static std::unique_ptr<PackFile> open(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

protected:
	FPX(const std::string& fullFilePath_, PackFileOptions options_);

	[[nodiscard]] static std::unique_ptr<PackFile> openInternal(const std::string& path, PackFileOptions options = {}, const Callback& callback = nullptr);

private:
	using VPK::generateKeyPairFiles;
	using VPK::sign;
	using VPK::getVersion;
	using VPK::setVersion;

	VPKEDIT_REGISTER_PACKFILE_OPEN(FPX_EXTENSION, &FPX::open);
};

} // namespace vpkedit
