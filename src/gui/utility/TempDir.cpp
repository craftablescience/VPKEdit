#include "TempDir.h"

#include <string_view>

#include <sourcepp/String.h>

using namespace sourcepp;

namespace {

constexpr std::string_view TMP_DIR_BASE = "vpkedit-";

} // namespace

TempDir::TempDir() : uuid(string::generateUUIDv4().c_str()) {
	QDir::temp().mkdir(TMP_DIR_BASE.data() + this->uuid);
	createdTempDirs() << this->dir();
}

QDir TempDir::dir() const {
	return QDir::tempPath() + QDir::separator() + TMP_DIR_BASE.data() + this->uuid;
}

QString TempDir::path() const {
	return this->dir().path();
}

// Deleted in Window - we need to delay deletion because of some platforms like KDE
QList<QDir>& TempDir::createdTempDirs() {
	static QList<QDir> dirs;
	return dirs;
}
