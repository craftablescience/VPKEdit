#include <vpkeditc/StringArray.h>

#include <cstdlib>

VPKEDIT_API VPKEdit_StringArray_t vpkedit_new_string_array(size_t size) {
	VPKEdit_StringArray_t array;
	if (size > 0) {
		array.size = size;
		array.data = static_cast<char**>(std::malloc(sizeof(char*) * size));
	} else {
		array.size = 0;
		array.data = nullptr;
	}
	return array;
}

VPKEDIT_API void vpkedit_delete_string_array(VPKEdit_StringArray_t* array) {
	if (array->data) {
		for (size_t i = 0; i < array->size; i++) {
			if (char* str = array->data[i]) {
				std::free(str);
				array->data[i] = nullptr;
			}
		}
		std::free(array->data);
		array->data = nullptr;
	}
	array->size = 0;
}
