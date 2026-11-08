#pragma once

#include "VEntryTreeView.hpp"

#include <QStackedWidget>

class VPreviewDirectoryParent;
class VPreviewInfo;

class VPreviewParent : public QStackedWidget {
	Q_OBJECT;

public:
	explicit VPreviewParent(QWidget* parent = nullptr);

	template<typename Self>
	[[nodiscard]] auto* previewDirectoryParent(this Self&& self) {
		return self.previewDirectory;
	}

	template<typename Self>
	[[nodiscard]] auto* previewPluginLoader(this Self&& self) {
		return self.previewPlugins;
	}

	void clearPreview();

public slots:
	void onDirSelected(const QString& path, vpkpp::Attribute attrs, const QStringList& subdirs, const QMap<QString, std::optional<vpkpp::Entry>>& subfiles);

	void onFileSelected(const QString& path, const QByteArray& data);

protected:
	VPreviewInfo* previewNone;
	VPreviewInfo* previewInfo;
	VPreviewDirectoryParent* previewDirectory;
	VPreviewPluginLoader* previewPlugins;
};
