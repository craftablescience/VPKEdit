#pragma once

#include <string>
#include <vector>

#include <vpkedit/Options.h>
#include <vpkeditc/Entry.h>
#include <vpkeditc/PackFile.h>
#include <vpkeditc/String.h>

namespace vpkedit {

class Entry;
class PackFile;

} // namespace vpkedit

#define VPKEDIT_EARLY_RETURN(var) \
	do {                          \
		if (!var) {               \
			return;               \
		}                         \
	} while (0)

#define VPKEDIT_EARLY_RETURN_VALUE(var, value) \
	do {                                       \
		if (!var) {                            \
			return value;                      \
		}                                      \
	} while (0)

vpkedit::PackFile* getPackFile(VPKEdit_PackFileHandle_t handle);

vpkedit::Entry* getEntry(VPKEdit_EntryHandle_t handle);

size_t writeStringToBuffer(std::string_view str, char* buffer, size_t bufferLen);

size_t writeVectorToBuffer(const std::vector<std::byte>& vec, unsigned char* buffer, size_t bufferLen);

VPKEdit_String_t createString(std::string_view str);

VPKEdit_Buffer_t createBuffer(const std::vector<std::byte>& vec);

VPKEdit_StringArray_t convertStringVector(const std::vector<std::string>& stringVec);

vpkedit::PackFileOptions convertOptionsFromC(VPKEdit_PackFileOptions_t options);

VPKEdit_PackFileOptions_t convertOptionsToC(vpkedit::PackFileOptions options);
