#pragma once

#include "widgets/VEntryTreeView.hpp"

#include <QStackedWidget>

class VPreviewDirectoryCompact;
class VPreviewDirectoryDetails;

class VPreviewDirectoryParent : public QStackedWidget {
	Q_OBJECT;

public:
	enum class ViewMode {
		ICONS,
		COMPACT,
		DETAILS,
	};

	explicit VPreviewDirectoryParent(QWidget* parent = nullptr);

	void populate(const QString& path, vpkpp::Attribute attrs, const QStringList& subdirs, const QMap<QString, std::optional<vpkpp::Entry>>& subfiles) const;

public slots:
	void onDirViewModeUpdated(VPreviewDirectoryParent::ViewMode mode);

	void onProcessedThumbnail(const QString& path, const QIcon& thumbnail) const;

protected:
	VPreviewDirectoryCompact* previewCompact;
	VPreviewDirectoryDetails* previewDetails;
};
