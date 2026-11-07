#pragma once

#include <QDialog>

#include "plugins/VPluginLoader.hpp"

class VPluginDialog : public QDialog {
	Q_OBJECT;

public:
	explicit VPluginDialog(const QMap<QString, QList<VPluginLoader::Metadata>>& pluginMetadata, QWidget* parent = nullptr);

	static void showPlugins(const QMap<QString, QList<VPluginLoader::Metadata>>& pluginMetadata, QWidget* parent = nullptr);
};
