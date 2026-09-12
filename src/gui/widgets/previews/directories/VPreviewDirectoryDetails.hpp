#pragma once

#include "widgets/previews/VPreviewDirectoryParent.hpp"

#include <QTableWidget>

class VPreviewDirectoryDetails : public QTableWidget {
	Q_OBJECT;

public:
	enum class Column : int {
		NAME = 0,
		TYPE,
		SIZE,
		PRELOAD_DATA_SIZE,
		ARCHIVE_INDEX,
		CRC32,
		MD5,
		COLUMN_COUNT,
	};

	explicit VPreviewDirectoryDetails(QWidget* parent = nullptr);

	void populate(const QString& path_, vpkpp::Attribute attrs, const QStringList& subdirs, const QMap<QString, std::optional<vpkpp::Entry>>& subfiles);

public slots:
	void onProcessedThumbnail(const QString& filepath, const QIcon& thumbnail) const;

protected:
	QString path;
};
