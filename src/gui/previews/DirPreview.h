#pragma once

#include <QTableWidget>

namespace vpkedit {

class VPK;

} // namespace vpkedit

class FileViewer;
class Window;

class DirPreview : public QTableWidget {
    Q_OBJECT;

public:
    DirPreview(FileViewer* fileViewer_, Window* window_, QWidget* parent = nullptr);

    void setPath(const QString& currentDir, const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpkedit::VPK& vpk);

    void addEntry(const vpkedit::VPK& vpk, const QString& path);

    void removeFile(const QString& path);

    void removeDir(const QString& path);

    void setSearchQuery(const QString& query);

    [[nodiscard]] const QString& getCurrentPath() const;

protected:
	void keyPressEvent(QKeyEvent* event) override;

private:
    void addRowForFile(const vpkedit::VPK& vpk, const QString& path);

    void addRowForDir(const QString& name);

    QString getItemPath(QTableWidgetItem* item) const;

    FileViewer* fileViewer;
    Window* window;

    QString currentPath;
    QString currentSearchQuery;
};
