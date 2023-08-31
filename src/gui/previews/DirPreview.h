#pragma once

#include <QTableWidget>

namespace vpktool {

class VPK;

} // namespace vpktool

class FileViewer;

class DirPreview : public QTableWidget {
    Q_OBJECT;

public:
    explicit DirPreview(FileViewer* fileViewer_, QWidget* parent = nullptr);

    void setPath(const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpktool::VPK& vpk);

private:
    FileViewer* fileViewer;
};
