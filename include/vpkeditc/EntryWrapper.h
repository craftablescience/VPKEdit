#pragma once

#include "API.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* VPKEdit_EntryHandle_t;

typedef struct {
	size_t size;
	VPKEdit_EntryHandle_t* data;
} VPKEdit_EntryHandleArray_t;

#ifdef __cplusplus
} // extern "C"
#endif

VPKEDIT_API size_t vpkedit_entry_get_path(VPKEdit_EntryHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_entry_get_parent_path(VPKEdit_EntryHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_entry_get_filename(VPKEdit_EntryHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_entry_get_stem(VPKEdit_EntryHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_entry_get_extension(VPKEdit_EntryHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API void vpkedit_entry_free(VPKEdit_EntryHandle_t* handle);

VPKEDIT_API void vpkedit_entry_array_free(VPKEdit_EntryHandleArray_t* array);
