#include "FileViewer.h"

#include <filesystem>

#include <QHBoxLayout>

#include "previews/DirPreview.h"
#include "previews/ImagePreview.h"
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

    this->imagePreview = new ImagePreview(this);
    layout->addWidget(this->imagePreview);

    this->textPreview = new TextPreview(this);
    layout->addWidget(this->textPreview);

    this->vtfPreview = new VTFPreview(this);
    layout->addWidget(this->vtfPreview);

    this->clearContents();
    this->setTextPreviewVisible();
}

void FileViewer::displayEntry(const QString& path) {
    QString extension(std::filesystem::path(path.toLower().toStdString()).extension().string().c_str());
    this->clearContents();
    if (ImagePreview::EXTENSIONS.contains(extension)) {
        // Image
        this->imagePreview->setImage(this->window->readBinaryEntry(path));
        this->setImagePreviewVisible();
    } else if (TextPreview::EXTENSIONS.contains(extension)) {
        // Text
        this->textPreview->setText(this->window->readTextEntry(path));
        this->setTextPreviewVisible();
    } else if (VTFPreview::EXTENSIONS.contains(extension)) {
        // VTF (texture)
        this->vtfPreview->setImage(this->window->readBinaryEntry(path));
        this->setVTFPreviewVisible();
    }
}

void FileViewer::displayDir(const QString& /*path*/, const QList<QString>& subfolders, const QList<QString>& entryPaths, const VPK& vpk) {
    this->clearContents();
    this->dirPreview->setPath(subfolders, entryPaths, vpk);
    this->setDirPreviewVisible();
}

void FileViewer::selectSubItemInDir(const QString& name) {
    this->window->selectSubItemInDir(name);
}

void FileViewer::clearContents() {
    this->textPreview->setText("");
    this->setTextPreviewVisible();
}

void FileViewer::setDirPreviewVisible() {
    this->dirPreview->show();
    this->imagePreview->hide();
    this->textPreview->hide();
    this->vtfPreview->hide();
}

void FileViewer::setImagePreviewVisible() {
    this->dirPreview->hide();
    this->imagePreview->show();
    this->textPreview->hide();
    this->vtfPreview->hide();
}

void FileViewer::setTextPreviewVisible() {
    this->dirPreview->hide();
    this->imagePreview->hide();
    this->textPreview->show();
    this->vtfPreview->hide();
}

void FileViewer::setVTFPreviewVisible() {
    this->dirPreview->hide();
    this->imagePreview->hide();
    this->textPreview->hide();
    this->vtfPreview->show();
}
