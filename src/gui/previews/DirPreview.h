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
    explicit DirPreview(FileViewer* fileViewer_, Window* window_, QWidget* parent = nullptr);

    void setPath(const QString& currentDir, const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpkedit::VPK& vpk);

    void setSearchQuery(const QString& query);

private:
    QString getItemPath(QTableWidgetItem* item);

    FileViewer* fileViewer;
    Window* window;

    QString currentPath;
    QString currentSearchQuery;
};
