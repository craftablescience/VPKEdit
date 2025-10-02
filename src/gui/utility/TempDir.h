#pragma once

#include <QDir>

/// Creates a new temporary directory. On window close the contents of the directory are
/// attempted to be deleted (although deletion won't happen for any files in use).
class TempDir {
public:
	TempDir();

	[[nodiscard]] QDir dir() const;

	[[nodiscard]] QString path() const;

	// Deleted in Window - we need to delay deletion because of some platforms like KDE
	[[nodiscard]] static QList<QDir>& createdTempDirs();

private:
	QString uuid;
};
