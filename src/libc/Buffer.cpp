#include <vpkeditc/Buffer.h>

#include <cstdlib>

VPKEDIT_API VPKEdit_Buffer_t vpkedit_buffer_new(size_t size) {
	VPKEdit_Buffer_t buffer;
	if (size > 0) {
		buffer.size = static_cast<int64_t>(size);
		buffer.data = static_cast<uint8_t*>(std::malloc(sizeof(uint8_t) * size));
	} else {
		buffer.size = 0;
		buffer.data = nullptr;
	}
	return buffer;
}

VPKEDIT_API void vpkedit_buffer_free(VPKEdit_Buffer_t* buffer) {
	if (buffer->data) {
		std::free(buffer->data);
		buffer->data = nullptr;
	}
	buffer->size = 0;
}
