#pragma once

#include <functional>
#include <optional>

#include <QPluginLoader>

#include "plugins/previews/VPreviewPlugin.hpp"
#include "plugins/thumbnails/VThumbnailPlugin.hpp"

class QStackedWidget;

struct VPluginMetadata {
	QString name;
	QString version;
	QString author;
	QString description;
	QMap<QString, QString> links;

	[[nodiscard]] static VPluginMetadata get(QPluginLoader* loader);
};

class VPluginLoaderBase : public QObject {
	Q_OBJECT;

public:
	using QObject::QObject;

	[[nodiscard]] QList<VPluginMetadata> getPluginMetadata();

signals:
	void clearedPlugins();

	void addedPlugin(const VPluginMetadata& metadata);

protected:
	static void iterate(const QString& subdir, const std::function<void(const QString&)>& callback);

	QList<QPluginLoader*> pluginLoaders;
};

class VPreviewPluginLoader : public VPluginLoaderBase {
	Q_OBJECT;

public:
	explicit VPreviewPluginLoader(QWidget* parent = nullptr);

	void reloadPlugins(VWindowAccess_V6_0* windowAccess);

	[[nodiscard]] bool hasPreview(const QString& path) const;

	[[nodiscard]] std::optional<QString> loadPreview(const QString& path, const quint8* dataPtr, quint64 dataLen) const;

	template<typename Self>
	[[nodiscard]] auto* previewStackedWidget(this Self&& self) {
		return self.previewStackedWidget_;
	}

protected:
	QMap<QString, QList<VPreviewPlugin_V6_0*>> previewPlugins;
	QMap<VPreviewPlugin_V6_0*, QWidget*> previewWidgets;
	QStackedWidget* previewStackedWidget_;
};

class VThumbnailPluginLoader : public VPluginLoaderBase {
	Q_OBJECT;

public:
	using VPluginLoaderBase::VPluginLoaderBase;

	void reloadPlugins(VWindowAccess_V6_0* windowAccess);

	[[nodiscard]] bool hasThumbnail(const QString& path) const;

	[[nodiscard]] QPixmap getThumbnail(const QString& path, const quint8* dataPtr, quint64 dataLen) const;

protected:
	QMap<QString, QList<VThumbnailPlugin_V6_0*>> thumbnailPlugins;
};
