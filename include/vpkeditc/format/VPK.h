#pragma once

#include "../PackFile.h"

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_create_empty(const char* path);

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_create_empty_with_options(const char* path, VPKEdit_PackFileOptions_t options);

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_create_from_directory(const char* vpkPath, const char* contentPath, bool saveToDir);

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_create_from_directory_with_options(const char* vpkPath, const char* contentPath, bool saveToDir, VPKEdit_PackFileOptions_t options);

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_open(const char* path);

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_vpk_open_with_options(const char* path, VPKEdit_PackFileOptions_t options);

VPKEDIT_API bool vpkedit_vpk_generate_keypair_files(const char* path);

VPKEDIT_API bool vpkedit_vpk_sign_from_file(VPKEdit_PackFileHandle_t handle, const char* filename);

VPKEDIT_API bool vpkedit_vpk_sign_from_mem(VPKEdit_PackFileHandle_t handle, const unsigned char* privateKeyBuffer, size_t privateKeyLen, const unsigned char* publicKeyBuffer, size_t publicKeyLen);

VPKEDIT_API uint32_t vpkedit_vpk_get_version(VPKEdit_PackFileHandle_t handle);

VPKEDIT_API void vpkedit_vpk_set_version(VPKEdit_PackFileHandle_t handle, uint32_t version);
