#pragma once

#include "API.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bool gma_writeCRCs;
	uint32_t vpk_version;
	uint32_t vpk_preferredChunkSize;
	bool vpk_generateMD5Entries;
	uint16_t zip_compressionMethod;
} VPKEdit_PackFileOptionsWrapper_t;

#ifdef __cplusplus
} // extern "C"
#endif
