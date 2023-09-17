#pragma once

#include <QTableWidget>

namespace vpkedit {

class VPK;

} // namespace vpkedit

class FileViewer;

class DirPreview : public QTableWidget {
    Q_OBJECT;

public:
    explicit DirPreview(FileViewer* fileViewer_, QWidget* parent = nullptr);

    void setPath(const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpkedit::VPK& vpk);

private:
    FileViewer* fileViewer;
};
