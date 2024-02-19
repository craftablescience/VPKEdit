#pragma once

#include "API.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	size_t size;
	char** data;
} VPKEdit_StringArray_t;

#ifdef __cplusplus
} // extern "C"
#endif

VPKEDIT_API VPKEdit_StringArray_t vpkedit_new_string_array(size_t size);

VPKEDIT_API void vpkedit_delete_string_array(VPKEdit_StringArray_t* array);
