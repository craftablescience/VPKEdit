#include <vpkeditc/PackFileWrapper.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <utility>

#include <vpkedit/PackFile.h>

#include "Helpers.hpp"

using namespace vpkedit;

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_open(const char* path) {
	VPKEDIT_EARLY_RETURN_VALUE(path, nullptr);

	auto packFile = PackFile::open(path);
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_open_with_options(const char* path, VPKEdit_PackFileOptionsWrapper_t options) {
	VPKEDIT_EARLY_RETURN_VALUE(path, nullptr);

	auto packFile = PackFile::open(path, ::convertOptionsFromC(options));
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API VPKEdit_PackFileTypeWrapper_e vpkedit_get_type(VPKEdit_PackFileHandle_t handle) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, VPKEDIT_PACK_FILE_TYPE_UNKNOWN);

	return static_cast<VPKEdit_PackFileTypeWrapper_e>(::getPackFile(handle)->getType());
}

VPKEDIT_API VPKEdit_PackFileOptionsWrapper_t vpkedit_get_options(VPKEdit_PackFileHandle_t handle) {
	return ::convertOptionsToC(::getPackFile(handle)->getOptions());
}

VPKEDIT_API VPKEdit_StringArray_t vpkedit_verify_entry_checksums(VPKEdit_PackFileHandle_t handle) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, (VPKEdit_StringArray_t{.size = 0, .data = nullptr}));

	return ::convertStringVector(::getPackFile(handle)->verifyEntryChecksums());
}

VPKEDIT_API bool vpkedit_verify_file_checksum(VPKEdit_PackFileHandle_t handle) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, false);

	return ::getPackFile(handle)->verifyFileChecksum();
}

VPKEDIT_API bool vpkedit_is_case_sensitive(VPKEdit_PackFileHandle_t handle) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, false);

	return ::getPackFile(handle)->isCaseSensitive();
}

VPKEDIT_API VPKEdit_EntryHandle_t vpkedit_find_entry(VPKEdit_PackFileHandle_t handle, const char* filename, bool includeUnbaked) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, nullptr);
	VPKEDIT_EARLY_RETURN_VALUE(filename, nullptr);

	auto entry = ::getPackFile(handle)->findEntry(filename, includeUnbaked);
	if (!entry) {
		return nullptr;
	}
	return new Entry(std::move(*entry));
}

VPKEDIT_API size_t vpkedit_read_entry(VPKEdit_PackFileHandle_t handle, VPKEdit_EntryHandle_t entry, unsigned char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(entry, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	if (auto binary = ::getPackFile(handle)->readEntry(*::getEntry(entry))) {
		return ::writeVectorToBuffer(*binary, buffer, bufferLen);
	}
	return 0;
}

VPKEDIT_API size_t vpkedit_read_entry_text(VPKEdit_PackFileHandle_t handle, VPKEdit_EntryHandle_t entry, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(entry, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	if (auto text = ::getPackFile(handle)->readEntryText(*::getEntry(entry))) {
		return ::writeStringToBuffer(*text, buffer, bufferLen);
	}
	buffer[0] = '\0';
	return 0;
}

VPKEDIT_API bool vpkedit_is_read_only(VPKEdit_PackFileHandle_t handle) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, false);

	return ::getPackFile(handle)->isReadOnly();
}

VPKEDIT_API void vpkedit_add_entry_from_file(VPKEdit_PackFileHandle_t handle, const char* filename, const char* pathToFile) {
	VPKEDIT_EARLY_RETURN(handle);
	VPKEDIT_EARLY_RETURN(filename);
	VPKEDIT_EARLY_RETURN(pathToFile);

	::getPackFile(handle)->addEntry(filename, pathToFile, {});
}

VPKEDIT_API void vpkedit_add_entry_from_mem(VPKEdit_PackFileHandle_t handle, const char* filename, const unsigned char* buffer, uint64_t bufferLen) {
	VPKEDIT_EARLY_RETURN(handle);
	VPKEDIT_EARLY_RETURN(filename);

	::getPackFile(handle)->addEntry(filename, reinterpret_cast<const std::byte*>(buffer), bufferLen, {});
}

VPKEDIT_API bool vpkedit_remove_entry(VPKEdit_PackFileHandle_t handle, const char* filename) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, false);
	VPKEDIT_EARLY_RETURN_VALUE(filename, false);

	return ::getPackFile(handle)->removeEntry(filename);
}

VPKEDIT_API bool vpkedit_bake(VPKEdit_PackFileHandle_t handle, const char* outputDir) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, false);
	VPKEDIT_EARLY_RETURN_VALUE(outputDir, false);

	return ::getPackFile(handle)->bake(outputDir, nullptr);
}

VPKEDIT_API VPKEdit_EntryHandleArray_t vpkedit_get_baked_entries(VPKEdit_PackFileHandle_t handle) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, (VPKEdit_EntryHandleArray_t{.size = 0, .data = nullptr}));

	std::vector<Entry*> heapEntries;
	for (const auto& [dir, entries] : ::getPackFile(handle)->getBakedEntries()) {
		for (const auto& entry : entries) {
			heapEntries.push_back(new Entry{entry});
		}
	}

	VPKEdit_EntryHandleArray_t array;
	array.size = heapEntries.size();
	array.data = static_cast<VPKEdit_EntryHandle_t*>(std::malloc(sizeof(VPKEdit_EntryHandle_t) * array.size));

	for (size_t i = 0; i < array.size; i++) {
		array.data[i] = heapEntries[i];
	}
	return array;
}

VPKEDIT_API VPKEdit_EntryHandleArray_t vpkedit_get_unbaked_entries(VPKEdit_PackFileHandle_t handle) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, (VPKEdit_EntryHandleArray_t{.size = 0, .data = nullptr}));

	std::vector<Entry*> heapEntries;
	for (const auto& [dir, entries] : ::getPackFile(handle)->getUnbakedEntries()) {
		for (const auto& entry : entries) {
			heapEntries.push_back(new Entry{entry});
		}
	}

	VPKEdit_EntryHandleArray_t array;
	array.size = heapEntries.size();
	array.data = static_cast<VPKEdit_EntryHandle_t*>(std::malloc(sizeof(VPKEdit_EntryHandle_t) * array.size));

	for (size_t i = 0; i < array.size; i++) {
		array.data[i] = heapEntries[i];
	}
	return array;
}

VPKEDIT_API size_t vpkedit_get_entry_count(VPKEdit_PackFileHandle_t handle, bool includeUnbaked) {
	return ::getPackFile(handle)->getEntryCount(includeUnbaked);
}

VPKEDIT_API size_t vpkedit_get_filepath(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getPackFile(handle)->getFilepath(), buffer, bufferLen);
}

VPKEDIT_API size_t vpkedit_get_truncated_filepath(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getPackFile(handle)->getTruncatedFilepath(), buffer, bufferLen);
}

VPKEDIT_API size_t vpkedit_get_filename(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getPackFile(handle)->getFilename(), buffer, bufferLen);
}

VPKEDIT_API size_t vpkedit_get_truncated_filename(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getPackFile(handle)->getTruncatedFilename(), buffer, bufferLen);
}

VPKEDIT_API size_t vpkedit_get_filestem(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getPackFile(handle)->getFilestem(), buffer, bufferLen);
}

VPKEDIT_API size_t vpkedit_get_truncated_filestem(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(::getPackFile(handle)->getTruncatedFilestem(), buffer, bufferLen);
}

VPKEDIT_API size_t vpkedit_get_supported_entry_attributes(VPKEdit_PackFileHandle_t handle, VPKEdit_Attribute_e* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	auto attrs = ::getPackFile(handle)->getSupportedEntryAttributes();
	for (size_t i = 0; i < bufferLen; i++) {
		if (i < attrs.size()) {
			buffer[i] = static_cast<VPKEdit_Attribute_e>(attrs[i]);
		} else {
			buffer[i] = VPKEDIT_ATTRIBUTE_NONE;
		}
	}
	return std::max(attrs.size(), bufferLen);
}

VPKEDIT_API size_t vpkedit_to_string(VPKEdit_PackFileHandle_t handle, char* buffer, size_t bufferLen) {
	VPKEDIT_EARLY_RETURN_VALUE(handle, 0);
	VPKEDIT_EARLY_RETURN_VALUE(buffer, 0);
	VPKEDIT_EARLY_RETURN_VALUE(bufferLen, 0);

	return ::writeStringToBuffer(std::string{*::getPackFile(handle)}, buffer, bufferLen);
}

VPKEDIT_API void vpkedit_close(VPKEdit_PackFileHandle_t* handle) {
	VPKEDIT_EARLY_RETURN(handle);

	std::default_delete<PackFile>()(::getPackFile(*handle));
	*handle = nullptr;
}

VPKEDIT_API VPKEdit_StringArray_t vpkedit_get_supported_file_types() {
	return ::convertStringVector(PackFile::getSupportedFileTypes());
}
