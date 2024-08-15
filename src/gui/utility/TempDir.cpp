#include "TempDir.h"

#include <string_view>

#include <sourcepp/String.h>

using namespace sourcepp;

constexpr std::string_view TMP_DIR_BASE = ".vpkedit-";

TempDir::TempDir()
		: uuid(string::generateUUIDv4().c_str()) {
	QDir::temp().mkdir(TMP_DIR_BASE.data() + this->uuid);
}

TempDir::~TempDir() {
	this->dir().removeRecursively();
}

QDir TempDir::dir() const {
	return QDir::tempPath() + QDir::separator() + TMP_DIR_BASE.data() + this->uuid;
}

QString TempDir::path() const {
	return this->dir().path();
}
