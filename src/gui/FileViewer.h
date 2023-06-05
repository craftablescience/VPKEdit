#pragma once

#include <QWidget>

class QTextEdit;

namespace vpktool {

class VPK;

} // namespace vpktool

class DirPreview;
class TextPreview;
class VTFPreview;
class Window;

class FileViewer : public QWidget {
    Q_OBJECT;

public:
    explicit FileViewer(Window* window_, QWidget* parent = nullptr);

    void displayEntry(const QString& path);

    void displayDir(const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpktool::VPK& vpk);

    void clearContents();

private:
    Window* window;

    DirPreview* dirPreview;
    TextPreview* textPreview;
    VTFPreview* imagePreview;

    void setDirPreviewVisible();
    void setTextPreviewVisible();
    void setImagePreviewVisible();
};
