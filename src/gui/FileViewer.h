#pragma once

#include <QWidget>

class QTextEdit;

namespace vpktool {

class VPK;

} // namespace vpktool

class DirPreview;
class ImagePreview;
class TextPreview;
class VTFPreview;
class Window;

class FileViewer : public QWidget {
    Q_OBJECT;

public:
    explicit FileViewer(Window* window_, QWidget* parent = nullptr);

    void displayEntry(const QString& path_);

    void displayDir(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths, const vpktool::VPK& vpk);

    void selectSubItemInDir(const QString& name);

    void clearContents();

private:
    Window* window;

    DirPreview* dirPreview;
    ImagePreview* imagePreview;
    TextPreview* textPreview;
    VTFPreview* vtfPreview;

    void setDirPreviewVisible();
    void setImagePreviewVisible();
    void setTextPreviewVisible();
    void setVTFPreviewVisible();
};
