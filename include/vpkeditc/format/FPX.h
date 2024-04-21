#pragma once

#include "../PackFile.h"

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_fpx_open(const char* path);

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_fpx_open_with_options(const char* path, VPKEdit_PackFileOptions_t options);

VPKEDIT_API bool vpkedit_fpx_generate_keypair_files(const char* path);

VPKEDIT_API bool vpkedit_fpx_sign(VPKEdit_PackFileHandle_t handle, const unsigned char* privateKeyBuffer, size_t privateKeyLen, const unsigned char* publicKeyBuffer, size_t publicKeyLen);
