#include "VPluginLoader.hpp"

#include <QApplication>
#include <QDir>
#include <QStackedWidget>
#include <QStandardPaths>

#include "Config.hpp"

namespace {

[[nodiscard]] QString getExtensionFromPath(const QString& path) {
	return QFileInfo{path}.suffix().toLower();
}

} // namespace

VPluginMetadata VPluginMetadata::get(QPluginLoader* loader) {
	VPluginMetadata metadata{
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

QList<VPluginMetadata> VPluginLoaderBase::getPluginMetadata() {
	QList<VPluginMetadata> out;
	for (auto* loader : this->pluginLoaders) {
		out.push_back(VPluginMetadata::get(loader));
	}
	return out;
}

void VPluginLoaderBase::iterate(const QString& subdir, const std::function<void(const QString&)>& callback) {
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

VPreviewPluginLoader::VPreviewPluginLoader(QWidget* parent) : VPluginLoaderBase{parent} {
	this->previewStackedWidget_ = new QStackedWidget{parent};
}

void VPreviewPluginLoader::reloadPlugins(VWindowAccess_V6_0* windowAccess) {
	while (this->previewStackedWidget()->count() > 0) {
		auto* widget = this->previewStackedWidget()->widget(0);
		this->previewStackedWidget()->removeWidget(widget);
		delete widget;
	}
	this->previewWidgets.clear();

	for (const auto& [name, pluginList] : this->previewPlugins.asKeyValueRange()) {
		for (const auto plugin : pluginList) {
			delete plugin;
		}
	}
	this->previewPlugins.clear();

	for (const auto* pluginLoader : this->pluginLoaders) {
		delete pluginLoader;
	}
	this->pluginLoaders.clear();

	emit this->clearedPlugins();

	VPluginLoaderBase::iterate(PROJECT_NAME "_previews", [this, windowAccess](const QString& libraryPath) {
		auto* loader = new QPluginLoader{libraryPath, this};
		if (auto* plugin = qobject_cast<VPreviewPlugin_V6_0*>(loader->instance())) {
			if (!loader->metaData().contains("MetaData") || !loader->metaData().value("MetaData").isObject()) {
				return;
			}
			this->pluginLoaders.push_back(loader);

			for (const auto& ext : plugin->getFileExtensions()) {
				if (!this->previewPlugins.contains(ext)) {
					this->previewPlugins[ext] = {plugin};
				} else {
					auto& pluginList = this->previewPlugins[ext];
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
			this->previewWidgets[plugin] = plugin->getPreview(this->previewStackedWidget());
			this->previewStackedWidget()->addWidget(this->previewWidgets[plugin]);
			emit this->addedPlugin(VPluginMetadata::get(loader));
		} else {
			loader->deleteLater();
		}
	});
}

bool VPreviewPluginLoader::hasPreview(const QString& path) const {
	return this->previewPlugins.contains(::getExtensionFromPath(path));
}

std::optional<QString> VPreviewPluginLoader::loadPreview(const QString& path, const quint8* dataPtr, quint64 dataLen) const {
	const auto ext = ::getExtensionFromPath(path);
	if (!this->previewPlugins.contains(ext)) {
		return std::nullopt;
	}
	QString errorMsg;
	for (int i = 0; i < this->previewPlugins[ext].size(); i++) {
		auto* plugin = this->previewPlugins[ext][i];
		errorMsg = plugin->setData(this->previewWidgets[plugin], path, dataPtr, dataLen);
		if (errorMsg.isEmpty()) {
			this->previewStackedWidget()->setCurrentWidget(this->previewWidgets[plugin]);
			return errorMsg;
		}
	}
	return errorMsg;
}

void VThumbnailPluginLoader::reloadPlugins(VWindowAccess_V6_0* windowAccess) {
	for (const auto& [name, pluginList] : this->thumbnailPlugins.asKeyValueRange()) {
		for (const auto* plugin : pluginList) {
			delete plugin;
		}
	}
	this->thumbnailPlugins.clear();

	for (const auto* pluginLoader : this->pluginLoaders) {
		delete pluginLoader;
	}
	this->pluginLoaders.clear();

	emit this->clearedPlugins();

	VPluginLoaderBase::iterate(PROJECT_NAME "_thumbnails", [this, windowAccess](const QString& libraryPath) {
		auto* loader = new QPluginLoader{libraryPath, this};
		if (auto* plugin = qobject_cast<VThumbnailPlugin_V6_0*>(loader->instance())) {
			if (!loader->metaData().contains("MetaData") || !loader->metaData().value("MetaData").isObject()) {
				return;
			}
			this->pluginLoaders.push_back(loader);

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
			emit this->addedPlugin(VPluginMetadata::get(loader));
		} else {
			loader->deleteLater();
		}
	});
}

bool VThumbnailPluginLoader::hasThumbnail(const QString& path) const {
	return this->thumbnailPlugins.contains(::getExtensionFromPath(path));
}

QPixmap VThumbnailPluginLoader::getThumbnail(const QString& path, const quint8* dataPtr, quint64 dataLen) const {
	const auto ext = ::getExtensionFromPath(path);
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
