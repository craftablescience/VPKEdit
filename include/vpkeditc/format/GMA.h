#pragma once

#include "../PackFile.h"

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_gma_open(const char* path);

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_gma_open_with_options(const char* path, VPKEdit_PackFileOptions_t options);
