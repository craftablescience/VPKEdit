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

VPluginLoader::Metadata VPluginLoader::Metadata::get(QPluginLoader* loader) {
	Metadata metadata{
		.name = loader->instance()->metaObject()->className(),
	};

	if (!loader->metaData().value("MetaData").isObject()) {
		return metadata;
	}

	const auto& pluginData = loader->metaData().value("MetaData").toObject();

	if (pluginData.contains("name") && pluginData.value("name").isString()) {
		metadata.name = pluginData.value("name").toString();
	}
	if (pluginData.contains("versionMajor")) {
		metadata.version = QString{"%1"}.arg(pluginData.value("versionMajor").toInt(0));
	} else {
		metadata.version = "0";
	}
	if (pluginData.contains("versionMinor")) {
		metadata.version += QString{".%1"}.arg(pluginData.value("versionMinor").toInt(0));
	} else {
		metadata.version += ".0";
	}
	if (pluginData.contains("versionPatch")) {
		metadata.version += QString{".%1"}.arg(pluginData.value("versionPatch").toInt(1));
	} else {
		metadata.version += ".1";
	}
	if (pluginData.contains("author") && pluginData.value("author").isString()) {
		metadata.author = pluginData.value("author").toString();
	}
	if (pluginData.contains("description") && pluginData.value("description").isString()) {
		metadata.description = pluginData.value("description").toString();
	}
	if (pluginData.contains("links") && pluginData.value("links").isObject()) {
		for (
			const auto& links = pluginData.value("links").toObject();
			const auto& linkName : links.keys()
		) {
			if (links.value(linkName).isString()) {
				metadata.links[linkName] = links.value(linkName).toString();
			}
		}
	}

	return metadata;
}

void VThumbnailPluginLoader::reloadPlugins(VWindowAccess_V6_0* windowAccess) {
	for (const auto& [name, pluginList] : this->thumbnailPlugins.asKeyValueRange()) {
		for (const auto* plugin : pluginList) {
			delete plugin;
		}
	}
	this->thumbnailPlugins.clear();

	for (const auto* pluginLoader : this->thumbnailPluginLoaders) {
		delete pluginLoader;
	}
	this->thumbnailPluginLoaders.clear();

	emit this->clearedThumbnailPlugins();

	VPluginLoader::iterate(PROJECT_NAME "_thumbnails", [this, windowAccess](const QString& libraryPath) {
		auto* loader = new QPluginLoader{libraryPath, this};
		if (auto* plugin = qobject_cast<VThumbnailPlugin_V6_0*>(loader->instance())) {
			if (!loader->metaData().contains("MetaData") || !loader->metaData().value("MetaData").isObject()) {
				return;
			}
			this->thumbnailPluginLoaders.push_back(loader);

			for (const auto& ext : plugin->getFileExtensions()) {
				if (!this->thumbnailPlugins.contains(ext)) {
					this->thumbnailPlugins[ext] = {plugin};
				} else {
					auto& pluginList = this->thumbnailPlugins[ext];
					bool addedPlugin = false;
					for (int i = 0; i < pluginList.size(); i++) {
						if (pluginList.at(i)->getPriority() < plugin->getPriority()) {
							pluginList.insert(i, plugin);
							addedPlugin = true;
							break;
						}
					}
					if (!addedPlugin) {
						pluginList.push_back(plugin);
					}
				}
			}
			plugin->initPlugin(windowAccess);
			emit this->addedThumbnailPlugin(VPluginLoader::Metadata::get(loader));
		} else {
			loader->deleteLater();
		}
	});
}

bool VThumbnailPluginLoader::hasThumbnail(const QString& path) const {
	const auto ext = QFileInfo{path}.suffix().toLower();
	return this->thumbnailPlugins.contains(ext);
}

QPixmap VThumbnailPluginLoader::getThumbnail(const QString& path, const quint8* dataPtr, quint64 dataLen) const {
	const auto ext = QFileInfo{path}.suffix().toLower();
	if (!this->thumbnailPlugins.contains(ext)) {
		return {};
	}
	for (auto* plugin : this->thumbnailPlugins[ext]) {
		if (const auto thumb = plugin->getThumbnail(path, dataPtr, dataLen); !thumb.isNull()) {
			return thumb;
		}
	}
	return {};
}

QList<VPluginLoader::Metadata> VThumbnailPluginLoader::getPluginMetadata() {
	QList<VPluginLoader::Metadata> out;
	for (auto* loader : this->thumbnailPluginLoaders) {
		out.push_back(VPluginLoader::Metadata::get(loader));
	}
	return out;
}
