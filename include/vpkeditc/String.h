#pragma once

#include "API.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int64_t size;
	char* data;
} VPKEdit_String_t;

#define VPKEDIT_STRING_INVALID (VPKEdit_String_t{.size = -1, .data = NULL})

typedef struct {
	int64_t size;
	char** data;
} VPKEdit_StringArray_t;

#define VPKEDIT_STRING_ARRAY_INVALID (VPKEdit_StringArray_t{.size = -1, .data = NULL})

#ifdef __cplusplus
} // extern "C"
#endif

VPKEDIT_API VPKEdit_String_t vpkedit_string_new(size_t size);

VPKEDIT_API void vpkedit_string_free(VPKEdit_String_t* str);

VPKEDIT_API VPKEdit_StringArray_t vpkedit_string_array_new(size_t size);

VPKEDIT_API void vpkedit_string_array_free(VPKEdit_StringArray_t* array);
