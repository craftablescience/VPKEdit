#include <vpkeditc/format/FPX.h>

#include <vpkedit/format/FPX.h>

#include "../Helpers.hpp"

using namespace vpkedit;

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_fpx_open(const char* path) {
	VPKEDIT_EARLY_RETURN_VALUE(path, nullptr);

	auto packFile = FPX::open(path);
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_fpx_open_with_options(const char* path, VPKEdit_PackFileOptions_t options) {
	VPKEDIT_EARLY_RETURN_VALUE(path, nullptr);

	auto packFile = FPX::open(path, ::convertOptionsFromC(options));
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API bool vpkedit_fpx_generate_keypair_files(const char* path) {
	VPKEDIT_EARLY_RETURN_VALUE(path, false);

	return FPX::generateKeyPairFiles(path);
}

VPKEDIT_API bool vpkedit_fpx_sign_from_file(VPKEdit_PackFileHandle_t handle, const char* filename) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, false);
	VPKEDIT_EARLY_RETURN_VALUE(filename, false);

	auto* fpx = ::getPackFile(handle);
	if (fpx->getType() != PackFileType::FPX) {
		return false;
	}
	return dynamic_cast<FPX*>(fpx)->sign(filename);
}

VPKEDIT_API bool vpkedit_fpx_sign_from_mem(VPKEdit_PackFileHandle_t handle, const unsigned char* privateKeyBuffer, size_t privateKeyLen, const unsigned char* publicKeyBuffer, size_t publicKeyLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, false);
	VPKEDIT_EARLY_RETURN_VALUE(privateKeyBuffer, false);
	VPKEDIT_EARLY_RETURN_VALUE(privateKeyLen, false);
	VPKEDIT_EARLY_RETURN_VALUE(publicKeyBuffer, false);
	VPKEDIT_EARLY_RETURN_VALUE(publicKeyLen, false);

	auto* fpx = ::getPackFile(handle);
	if (fpx->getType() != PackFileType::FPX) {
		return false;
	}
	return dynamic_cast<FPX*>(fpx)->sign(
		{reinterpret_cast<const std::byte*>(privateKeyBuffer), reinterpret_cast<const std::byte*>(privateKeyBuffer + privateKeyLen)},
		{reinterpret_cast<const std::byte*>(publicKeyBuffer), reinterpret_cast<const std::byte*>(publicKeyBuffer + publicKeyLen)});
}
