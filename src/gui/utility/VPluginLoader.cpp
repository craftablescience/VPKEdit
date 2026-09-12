#include "VPluginLoader.hpp"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>

#include "Config.hpp"

void VPluginLoader::iterate(const QString& subdir, const std::function<void(const QString&)>& callback) {
	auto pluginLocations = QCoreApplication::libraryPaths();
	pluginLocations << QApplication::applicationDirPath() + QDir::separator() + PROJECT_NAME;
	pluginLocations << QApplication::applicationDirPath() + QDir::separator() + ".." + QDir::separator() + "plugins";
#if defined(Q_OS_WIN)
	for (const auto& path : QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)) {
		pluginLocations << path + "/" + QString{PROJECT_NAME};
	}
#elif defined(Q_OS_APPLE)
	pluginLocations << QDir{"../PlugIns"}.absolutePath();
#elif defined(Q_OS_LINUX)
	pluginLocations << "/usr/lib/" + QString{PROJECT_NAME};
	pluginLocations << "/usr/lib64/" + QString{PROJECT_NAME};
	pluginLocations << "~/.local/lib/" + QString{PROJECT_NAME};
	pluginLocations << "~/.local/lib64/" + QString{PROJECT_NAME};
#endif
	for (const QString& dirPath : pluginLocations) {
		for (const QDir dir{dirPath + QDir::separator() + subdir}; const QString& libraryName : dir.entryList(QDir::Files)) {
			callback(dir.absoluteFilePath(libraryName));
		}
	}
}
