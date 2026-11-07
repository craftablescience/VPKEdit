#pragma once

#include <functional>

#include <QPluginLoader>

#include "plugins/thumbnails/VThumbnailPlugin.hpp"

namespace VPluginLoader {

void iterate(const QString& subdir, const std::function<void(const QString&)>& callback);

struct Metadata {
	QString name;
	QString version;
	QString author;
	QString description;
	QMap<QString, QString> links;

	[[nodiscard]] static Metadata get(QPluginLoader* loader);
};

} // namespace PluginFinder

class VThumbnailPluginLoader : public QObject {
	Q_OBJECT;

public:
	using QObject::QObject;

	void reloadPlugins(VWindowAccess_V6_0* windowAccess);

	[[nodiscard]] bool hasThumbnail(const QString& path) const;

	[[nodiscard]] QPixmap getThumbnail(const QString& path, const quint8* dataPtr, quint64 dataLen) const;

	[[nodiscard]] QList<VPluginLoader::Metadata> getPluginMetadata();

signals:
	void clearedThumbnailPlugins();

	void addedThumbnailPlugin(const VPluginLoader::Metadata& metadata);

protected:
	QList<QPluginLoader*> thumbnailPluginLoaders;
	QMap<QString, QList<VThumbnailPlugin_V6_0*>> thumbnailPlugins;
};
