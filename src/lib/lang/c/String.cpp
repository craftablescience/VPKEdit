#include <vpkeditc/String.h>

#include <cstdlib>

VPKEDIT_API VPKEdit_String_t vpkedit_string_new(size_t size) {
	VPKEdit_String_t str;
	if (size > 0) {
		str.size = static_cast<int64_t>(size);
		str.data = static_cast<char*>(std::malloc(sizeof(char) * (size + 1)));
		str.data[size] = '\0';
	} else {
		str.size = 0;
		str.data = nullptr;
	}
	return str;
}

VPKEDIT_API void vpkedit_string_free(VPKEdit_String_t* str) {
	if (str->data) {
		std::free(str->data);
		str->data = nullptr;
	}
	str->size = 0;
}

VPKEDIT_API VPKEdit_StringArray_t vpkedit_string_array_new(size_t size) {
	VPKEdit_StringArray_t array;
	if (size > 0) {
		array.size = static_cast<int64_t>(size);
		array.data = static_cast<char**>(std::malloc(sizeof(char*) * size));
	} else {
		array.size = 0;
		array.data = nullptr;
	}
	return array;
}

VPKEDIT_API void vpkedit_string_array_free(VPKEdit_StringArray_t* array) {
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
