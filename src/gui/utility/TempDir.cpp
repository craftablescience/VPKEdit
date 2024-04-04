#include "TempDir.h"

#include <string_view>

constexpr std::string_view VPKEDIT_TMP_DIR = ".vpkedit_tmp";

void TempDir::create() {
	QDir::temp().mkdir(VPKEDIT_TMP_DIR.data());
}

QDir TempDir::get() {
	create();
	return QDir::tempPath() + QDir::separator() + VPKEDIT_TMP_DIR.data();
}

void TempDir::clear() {
	get().removeRecursively();
}
