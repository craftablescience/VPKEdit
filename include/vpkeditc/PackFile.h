#pragma once

#include "Attribute.h"
#include "Entry.h"
#include "Options.h"
#include "PackFileType.h"
#include "StringArray.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* VPKEdit_PackFileHandle_t;

#ifdef __cplusplus
} // extern "C"
#endif

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_open(const char* path);

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_open_with_options(const char* path, VPKEdit_PackFileOptions_t options);

VPKEDIT_API VPKEdit_PackFileType_e vpkedit_get_type(VPKEdit_PackFileHandle_t handle);

VPKEDIT_API VPKEdit_PackFileOptions_t vpkedit_get_options(VPKEdit_PackFileHandle_t handle);

VPKEDIT_API VPKEdit_StringArray_t vpkedit_verify_entry_checksums(VPKEdit_PackFileHandle_t handle);

VPKEDIT_API bool vpkedit_verify_file_checksum(VPKEdit_PackFileHandle_t handle);

VPKEDIT_API bool vpkedit_is_case_sensitive(VPKEdit_PackFileHandle_t handle);

// REQUIRES MANUAL FREE: vpkedit_entry_free
VPKEDIT_API VPKEdit_EntryHandle_t vpkedit_find_entry(VPKEdit_PackFileHandle_t handle, const char* filename, bool includeUnbaked);

VPKEDIT_API size_t vpkedit_read_entry(VPKEdit_PackFileHandle_t handle, VPKEdit_EntryHandle_t entry, unsigned char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_read_entry_text(VPKEdit_PackFileHandle_t handle, VPKEdit_EntryHandle_t entry, char* buffer, size_t bufferLen);

VPKEDIT_API bool vpkedit_is_read_only(VPKEdit_PackFileHandle_t handle);

VPKEDIT_API void vpkedit_add_entry_from_file(VPKEdit_PackFileHandle_t handle, const char* filename, const char* pathToFile);

VPKEDIT_API void vpkedit_add_entry_from_mem(VPKEdit_PackFileHandle_t handle, const char* filename, const unsigned char* buffer, uint64_t bufferLen);

VPKEDIT_API bool vpkedit_remove_entry(VPKEdit_PackFileHandle_t handle, const char* filename);

VPKEDIT_API bool vpkedit_bake(VPKEdit_PackFileHandle_t handle, const char* outputDir);

// REQUIRES MANUAL FREE: vpkedit_entry_array_free
VPKEDIT_API VPKEdit_EntryHandleArray_t vpkedit_get_baked_entries(VPKEdit_PackFileHandle_t handle);

// REQUIRES MANUAL FREE: vpkedit_entry_array_free
VPKEDIT_API VPKEdit_EntryHandleArray_t vpkedit_get_unbaked_entries(VPKEdit_PackFileHandle_t handle);

VPKEDIT_API size_t vpkedit_get_entry_count(VPKEdit_PackFileHandle_t handle, bool includeUnbaked);

VPKEDIT_API size_t vpkedit_get_filepath(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_get_truncated_filepath(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_get_filename(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_get_truncated_filename(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_get_filestem(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_get_truncated_filestem(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_get_supported_entry_attributes(VPKEdit_PackFileHandle_t handle, VPKEdit_Attribute_e* buffer, size_t bufferLen);

VPKEDIT_API size_t vpkedit_to_string(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen);

VPKEDIT_API void vpkedit_close(VPKEdit_PackFileHandle_t* handle);

VPKEDIT_API VPKEdit_StringArray_t vpkedit_get_supported_file_types();
