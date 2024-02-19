#include <vpkeditc/format/GRPWrapper.h>

#include <vpkedit/format/GRP.h>

#include "../Helpers.hpp"

using namespace vpkedit;

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_grp_open(const char* path) {
	VPKEDIT_EARLY_RETURN_VALUE(path, nullptr);

	auto packFile = GRP::open(path);
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}

VPKEDIT_API VPKEdit_PackFileHandle_t vpkedit_grp_open_with_options(const char* path, VPKEdit_PackFileOptionsWrapper_t options) {
	VPKEDIT_EARLY_RETURN_VALUE(path, nullptr);

	auto packFile = GRP::open(path, ::convertOptionsFromC(options));
	if (!packFile) {
		return nullptr;
	}
	return packFile.release();
}
