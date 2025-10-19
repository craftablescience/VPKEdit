#pragma once

#include <QTableWidget>

namespace vpkpp {

class Entry;
class PackFile;

} // namespace vpkpp

class QKeyEvent;
class QMouseEvent;

class FileViewer;
class Window;

class DirPreview : public QTableWidget {
	Q_OBJECT;

public:
	DirPreview(FileViewer* fileViewer_, Window* window_, QWidget* parent = nullptr);

	void setPath(const QString& currentDir, const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpkpp::PackFile& packFile);

	void addEntry(const vpkpp::Entry& entry, const QString& path);

	void removeFile(const QString& path);

	void removeDir(const QString& path);

	void setSearchQuery(const QString& query);

	[[nodiscard]] const QString& getCurrentPath() const;

protected:
	void keyPressEvent(QKeyEvent* event) override;

	void mousePressEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

private:
	void addRowForFile(const vpkpp::Entry& entry, const QString& path);

	void addRowForDir(const QString& name);

	QString getItemPath(QTableWidgetItem* item) const;

	void removeSelectedRows(bool needsConfirmDialog);

	FileViewer* fileViewer;
	Window* window;

	QPoint dragStartPos;
	QList<QTableWidgetItem*> dragSelectedItems;

	QString currentPath;
	QString currentSearchQuery;
};
