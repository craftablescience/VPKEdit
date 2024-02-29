#include <vpkeditc/Entry.h>

#include <vpkedit/Entry.h>

#include "Helpers.hpp"

using namespace vpkedit;

VPKEDIT_API size_t vpkedit_entry_get_path(VPKEdit_EntryHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getEntry(handle)->path, buffer, bufferLen);
}

VPKEDIT_API size_t vpkedit_entry_get_parent_path(VPKEdit_EntryHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getEntry(handle)->getParentPath(), buffer, bufferLen);
}

VPKEDIT_API size_t vpkedit_entry_get_filename(VPKEdit_EntryHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getEntry(handle)->getFilename(), buffer, bufferLen);
}

VPKEDIT_API size_t vpkedit_entry_get_stem(VPKEdit_EntryHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getEntry(handle)->getStem(), buffer, bufferLen);
}

VPKEDIT_API size_t vpkedit_entry_get_extension(VPKEdit_EntryHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getEntry(handle)->getExtension(), buffer, bufferLen);
}

VPKEDIT_API void vpkedit_entry_free(VPKEdit_EntryHandle_t* handle) {
	VPKEDIT_EARLY_RETURN(handle);

	delete ::getEntry(*handle);
	*handle = nullptr;
}

VPKEDIT_API void vpkedit_entry_array_free(VPKEdit_EntryHandleArray_t* array) {
	VPKEDIT_EARLY_RETURN(array);

	if (array->data) {
		for (size_t i = 0; i < array->size; i++) {
			if (auto& entry = array->data[i]) {
				vpkedit_entry_free(&entry);
			}
		}
		std::free(array->data);
		array->data = nullptr;
	}
	array->size = 0;
}
