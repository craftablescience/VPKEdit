#pragma once

#include <QDir>

/// Creates a new temporary directory. On destruction the contents of the directory are
/// attempted to be deleted (although deletion won't happen for any files in use).
class TempDir {
public:
	TempDir();

	~TempDir();

	[[nodiscard]] QDir dir() const;

	[[nodiscard]] QString path() const;

private:
	QString uuid;
};
