#pragma once

#include "API.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* VPKEdit_EntryHandle_t;

typedef struct {
	int64_t size;
	VPKEdit_EntryHandle_t* data;
} VPKEdit_EntryHandleArray_t;

typedef void* VPKEdit_VirtualEntryHandle_t;

#define VPKEDIT_ENTRY_HANDLE_ARRAY_INVALID (VPKEdit_EntryHandleArray_t{.size = -1, .data = NULL})

typedef struct {
	int64_t size;
	VPKEdit_VirtualEntryHandle_t* data;
} VPKEdit_VirtualEntryHandleArray_t;

#define VPKEDIT_VIRTUAL_ENTRY_HANDLE_ARRAY_INVALID (VPKEdit_VirtualEntryHandleArray_t{.size = -1, .data = NULL})

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

VPKEDIT_API size_t vpkedit_virtual_entry_get_name(VPKEdit_VirtualEntryHandle_t* handle, char* buffer, size_t bufferLen);

VPKEDIT_API bool vpkedit_virtual_entry_is_writable(VPKEdit_VirtualEntryHandle_t* handle);

VPKEDIT_API void vpkedit_virtual_entry_free(VPKEdit_VirtualEntryHandle_t* handle);

VPKEDIT_API void vpkedit_virtual_entry_array_free(VPKEdit_VirtualEntryHandleArray_t* array);
