#include <vpkeditc/format/VPK.h>

#include <vpkedit/format/VPK.h>

#include "../Helpers.hpp"

using namespace vpkedit;

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_create_empty(const char* path) {
	VPKEDIT_EARLY_RETURN_VALUE(path, nullptr);

	auto packFile = VPK::createEmpty(path);
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_create_empty_with_options(const char* path, VPKEdit_PackFileOptions_t options) {
	VPKEDIT_EARLY_RETURN_VALUE(path, nullptr);

	auto packFile = PackFile::open(path, ::convertOptionsFromC(options));
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_create_from_directory(const char* vpkPath, const char* contentPath, bool saveToDir) {
	VPKEDIT_EARLY_RETURN_VALUE(vpkPath, nullptr);
	VPKEDIT_EARLY_RETURN_VALUE(contentPath, nullptr);

	auto packFile = VPK::createFromDirectory(vpkPath, contentPath, saveToDir);
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_create_from_directory_with_options(const char* vpkPath, const char* contentPath, bool saveToDir, VPKEdit_PackFileOptions_t options) {
	VPKEDIT_EARLY_RETURN_VALUE(vpkPath, nullptr);
	VPKEDIT_EARLY_RETURN_VALUE(contentPath, nullptr);

	auto packFile = VPK::createFromDirectory(vpkPath, contentPath, saveToDir, ::convertOptionsFromC(options));
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_open(const char* path) {
	VPKEDIT_EARLY_RETURN_VALUE(path, nullptr);

	auto packFile = VPK::open(path);
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_open_with_options(const char* path, VPKEdit_PackFileOptions_t options) {
	VPKEDIT_EARLY_RETURN_VALUE(path, nullptr);

	auto packFile = VPK::open(path, ::convertOptionsFromC(options));
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API bool vpkedit_vpk_generate_keypair_files(const char* path) {
	VPKEDIT_EARLY_RETURN_VALUE(path, false);

	return VPK::generateKeyPairFiles(path);
}

VPKEDIT_API bool vpkedit_vpk_sign(VPKEdit_PackFileHandle_t handle, const unsigned char* privateKeyBuffer, size_t privateKeyLen, const unsigned char* publicKeyBuffer, size_t publicKeyLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, false);
	VPKEDIT_EARLY_RETURN_VALUE(privateKeyBuffer, false);
	VPKEDIT_EARLY_RETURN_VALUE(privateKeyLen, false);
	VPKEDIT_EARLY_RETURN_VALUE(publicKeyBuffer, false);
	VPKEDIT_EARLY_RETURN_VALUE(publicKeyLen, false);

	auto* vpk = ::getPackFile(handle);
	if (vpk->getType() != PackFileType::VPK) {
		return false;
	}
	return dynamic_cast<VPK*>(vpk)->sign(
		{reinterpret_cast<const std::byte*>(privateKeyBuffer), reinterpret_cast<const std::byte*>(privateKeyBuffer + privateKeyLen)},
		{reinterpret_cast<const std::byte*>(publicKeyBuffer), reinterpret_cast<const std::byte*>(publicKeyBuffer + publicKeyLen)});
}

VPKEDIT_API uint32_t vpkedit_vpk_get_version(VPKEdit_PackFileHandle_t handle) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);

	auto* vpk = ::getPackFile(handle);
	if (vpk->getType() != PackFileType::VPK) {
		return 0;
	}
	return dynamic_cast<VPK*>(vpk)->getVersion();
}

VPKEDIT_API void vpkedit_vpk_set_version(VPKEdit_PackFileHandle_t handle, uint32_t version) {
	VPKEDIT_EARLY_RETURN(handle);

	auto* vpk = ::getPackFile(handle);
	if (vpk->getType() != PackFileType::VPK) {
		return;
	}
	dynamic_cast<VPK*>(vpk)->setVersion(version);
}
