#include "FileViewer.h"

#include <QHBoxLayout>

#include "previews/DirPreview.h"
#include "previews/TextPreview.h"
#include "previews/VTFPreview.h"
#include "Window.h"

using namespace vpktool;

FileViewer::FileViewer(Window* window_, QWidget* parent)
        : QWidget(parent)
        , window(window_) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    this->dirPreview = new DirPreview(this);
    layout->addWidget(this->dirPreview);

    this->textPreview = new TextPreview(this);
    layout->addWidget(this->textPreview);

    this->imagePreview = new VTFPreview(this);
    layout->addWidget(this->imagePreview);

    this->clearContents();
    this->setTextPreviewVisible();
}

void FileViewer::displayEntry(const QString& path) {
    this->clearContents();
    if (path.endsWith(".txt") ||
        path.endsWith(".md")  ||
        path.endsWith(".gi")  ||
        path.endsWith(".res") ||
        path.endsWith(".nut") ||
        path.endsWith(".lua") ||
        path.endsWith(".cfg") ||
        path.endsWith(".ini") ||
        path.endsWith(".kv")  ||
        path.endsWith(".kv3") ||
        path.endsWith(".vmf") || // hey you never know
        path.endsWith(".vmt")) {
        // It's text
        this->textPreview->setText(this->window->readTextEntry(path));
        this->setTextPreviewVisible();
    } else if (path.endsWith(".vtf")) {
        // VTF (image)
        this->imagePreview->setImage(this->window->readBinaryEntry(path));
        this->setImagePreviewVisible();
    }
}

void FileViewer::displayDir(const QList<QString>& subfolders, const QList<QString>& entryPaths, const VPK& vpk) {
    this->clearContents();
    this->dirPreview->setPath(subfolders, entryPaths, vpk);
    this->setDirPreviewVisible();
}

void FileViewer::clearContents() {
    this->textPreview->setText("");
    this->setTextPreviewVisible();
}

void FileViewer::setDirPreviewVisible() {
    this->dirPreview->show();
    this->textPreview->hide();
    this->imagePreview->hide();
}

void FileViewer::setTextPreviewVisible() {
    this->dirPreview->hide();
    this->textPreview->show();
    this->imagePreview->hide();
}

void FileViewer::setImagePreviewVisible() {
    this->dirPreview->hide();
    this->textPreview->hide();
    this->imagePreview->show();
}
