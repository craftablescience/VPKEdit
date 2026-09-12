#pragma once

#include "widgets/previews/VPreviewDirectoryParent.hpp"

#include <QListWidget>

class VPreviewDirectoryCompact : public QListWidget {
	Q_OBJECT;

public:
	explicit VPreviewDirectoryCompact(QWidget* parent = nullptr);

	void populate(const QString& path_, vpkpp::Attribute attrs, const QStringList& subdirs, const QMap<QString, std::optional<vpkpp::Entry>>& subfiles);

	[[nodiscard]] bool isCompact() const;

	[[nodiscard]] bool isIcons() const;

	void setCompactOrIcons(bool icons);

public slots:
	void onProcessedThumbnail(const QString& filepath, const QIcon& thumbnail) const;

protected:
	void resizeItems();

	QString path;
};
