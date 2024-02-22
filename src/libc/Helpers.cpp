#include "Helpers.hpp"

#include <cstring>

#include <vpkedit/PackFile.h>

using namespace vpkedit;

PackFile* getPackFile(VPKEdit_PackFileHandle_t handle) {
	return static_cast<PackFile*>(handle);
}

Entry* getEntry(VPKEdit_EntryHandle_t handle) {
	return static_cast<Entry*>(handle);
}

size_t writeStringToBuffer(std::string_view str, char* buffer, size_t bufferLen) {
	if (str.length() >= bufferLen) {
		std::memcpy(buffer, str.data(), bufferLen);
		buffer[bufferLen - 1] = '\0';
		return bufferLen;
	}
	std::memcpy(buffer, str.data(), str.length());
	buffer[str.length()] = '\0';
	return str.length() - 1;
}

size_t writeVectorToBuffer(const std::vector<std::byte>& vec, unsigned char* buffer, size_t bufferLen) {
	if (vec.size() >= bufferLen) {
		std::memcpy(buffer, vec.data(), bufferLen);
		return bufferLen;
	}
	std::memcpy(buffer, vec.data(), vec.size());
	return vec.size();
}

VPKEdit_StringArray_t convertStringVector(const std::vector<std::string>& stringVec) {
	auto array = vpkedit_new_string_array(stringVec.size());
	for (size_t i = 0; i < stringVec.size(); i++) {
		array.data[i] = static_cast<char*>(std::malloc(sizeof(char) * (stringVec[i].length() + 1)));
		std::memcpy(array.data[i], stringVec[i].c_str(), stringVec[i].length());
		array.data[i][stringVec[i].length()] = '\0';
	}
	return array;
}

vpkedit::PackFileOptions convertOptionsFromC(VPKEdit_PackFileOptionsWrapper_t options) {
	return {
		.gma_writeCRCs = options.gma_writeCRCs,
		.vpk_version = options.vpk_version,
		.vpk_preferredChunkSize = options.vpk_preferredChunkSize,
		.vpk_generateMD5Entries = options.vpk_generateMD5Entries,
		.zip_compressionMethod = options.zip_compressionMethod,
	};
}

VPKEdit_PackFileOptionsWrapper_t convertOptionsToC(vpkedit::PackFileOptions options) {
	return {
		.gma_writeCRCs = options.gma_writeCRCs,
		.vpk_version = options.vpk_version,
		.vpk_preferredChunkSize = options.vpk_preferredChunkSize,
		.vpk_generateMD5Entries = options.vpk_generateMD5Entries,
		.zip_compressionMethod = options.zip_compressionMethod,
	};
}
