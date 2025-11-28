#include "PluginFinder.h"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>

#include "Config.h"

void PluginFinder::doTheThing(const QString& subdir, const std::function<void(const QString&)>& callback) {
	QStringList pluginLocations{QApplication::applicationDirPath()};
#if defined(_WIN32)
	for (const auto& path : QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)) {
		pluginLocations << path + "/" + QString{PROJECT_NAME.data()};
	}
#elif defined(__APPLE__)
	pluginLocations << QDir{"../PlugIns"}.absolutePath();
#elif defined(__linux__)
	pluginLocations << "/usr/" VPKEDIT_LIBDIR "/" + QString{PROJECT_NAME.data()};
	pluginLocations << QDir{"~/.local/" VPKEDIT_LIBDIR "/" + QString{PROJECT_NAME.data()}}.canonicalPath();
#endif
	for (const QString& dirPath : pluginLocations) {
		for (const QDir dir{dirPath + QDir::separator() + subdir}; const QString& libraryName : dir.entryList(QDir::Files)) {
			callback(dir.absoluteFilePath(libraryName));
		}
	}
}
