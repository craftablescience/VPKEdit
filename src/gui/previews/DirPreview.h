#pragma once

#include <QTableWidget>

namespace vpkedit {

class PackFile;

} // namespace vpkedit

class QKeyEvent;

class FileViewer;
class Window;

class DirPreview : public QTableWidget {
    Q_OBJECT;

public:
    DirPreview(FileViewer* fileViewer_, Window* window_, QWidget* parent = nullptr);

    void setPath(const QString& currentDir, const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpkedit::PackFile& packFile);

    void addEntry(const vpkedit::PackFile& packFile, const QString& path);

    void removeFile(const QString& path);

    void removeDir(const QString& path);

    void setSearchQuery(const QString& query);

    [[nodiscard]] const QString& getCurrentPath() const;

protected:
	void keyPressEvent(QKeyEvent* event) override;

private:
    void addRowForFile(const vpkedit::PackFile& packFile, const QString& path, bool isVPK);

    void addRowForDir(const QString& name, bool isVPK);

    QString getItemPath(QTableWidgetItem* item) const;

    FileViewer* fileViewer;
    Window* window;

    QString currentPath;
    QString currentSearchQuery;
};
