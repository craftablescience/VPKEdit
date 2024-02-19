#pragma once

#include "../PackFileWrapper.h"

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_zip_open(const char* path);

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_zip_open_with_options(const char* path, VPKEdit_PackFileOptionsWrapper_t options);
