#pragma once

#include <QDialog>

#include "plugins/VPluginLoader.hpp"

class VPluginDialog : public QDialog {
	Q_OBJECT;

public:
	explicit VPluginDialog(const QMap<QString, QList<VPluginMetadata>>& pluginMetadata, QWidget* parent = nullptr);

	static void showPlugins(const QMap<QString, QList<VPluginMetadata>>& pluginMetadata, QWidget* parent = nullptr);
};
