#pragma once

#include <QTableWidget>

namespace vpktool {

class VPK;

} // namespace vpktool

class Window;

class DirPreview : public QTableWidget {
    Q_OBJECT;

public:
    explicit DirPreview(QWidget* parent = nullptr);

    void setPath(const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpktool::VPK& vpk);
};
