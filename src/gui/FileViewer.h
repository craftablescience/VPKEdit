#pragma once

#include <QWidget>

class QTextEdit;

class VTFPreview;
class Window;

class FileViewer : public QWidget {
    Q_OBJECT;

public:
    explicit FileViewer(Window* window_, QWidget* parent = nullptr);

    void displayEntry(const QString& path);

    void clearContents();

private:
    Window* window;

    QTextEdit* textEdit;
    VTFPreview* image;

    void setTextEditVisible();
    void setImageVisible();
};
