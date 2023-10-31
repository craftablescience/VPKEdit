#include "FileViewer.h"

#include <filesystem>

#include <QHBoxLayout>
#include <QMessageBox>

#include "previews/DirPreview.h"
#include "previews/ErrorPreview.h"
#include "previews/ImagePreview.h"
#include "previews/TextPreview.h"
#include "previews/VTFPreview.h"
#include "Window.h"

using namespace vpkedit;

FileViewer::FileViewer(Window* window_, QWidget* parent)
        : QWidget(parent)
        , window(window_) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* dirPreview = newPreview<DirPreview>(this, this->window, this);
    layout->addWidget(dirPreview);

    auto* errorPreview = newPreview<ErrorPreview>(this);
    layout->addWidget(errorPreview);

    auto* imagePreview = newPreview<ImagePreview>(this);
    layout->addWidget(imagePreview);

    auto* textPreview = newPreview<TextPreview>(this);
    layout->addWidget(textPreview);

    auto* vtfPreview = newPreview<VTFPreview>(this);
    layout->addWidget(vtfPreview);

    this->clearContents();
}

void FileViewer::displayEntry(const QString& path) {
    QString extension(std::filesystem::path(path.toLower().toStdString()).extension().string().c_str());
    this->clearContents();
    if (ImagePreview::EXTENSIONS.contains(extension)) {
        // Image
        auto binary = this->window->readBinaryEntry(path);
        if (!binary) {
            this->showPreview<ErrorPreview>();
            return;
        }
        this->getPreview<ImagePreview>()->setImage(*binary);
        this->showPreview<ImagePreview>();
    } else if (TextPreview::EXTENSIONS.contains(extension)) {
        // Text
        auto text = this->window->readTextEntry(path);
        if (!text) {
            this->showPreview<ErrorPreview>();
            return;
        }
        this->getPreview<TextPreview>()->setText(*text);
        this->showPreview<TextPreview>();
    } else if (VTFPreview::EXTENSIONS.contains(extension)) {
        // VTF (texture)
        auto binary = this->window->readBinaryEntry(path);
        if (!binary) {
            this->showPreview<ErrorPreview>();
            return;
        }
        this->getPreview<VTFPreview>()->setImage(*binary);
        this->showPreview<VTFPreview>();
    }
}

void FileViewer::displayDir(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths, const VPK& vpk) {
    this->clearContents();
    this->getPreview<DirPreview>()->setPath(path, subfolders, entryPaths, vpk);
    this->showPreview<DirPreview>();
}

void FileViewer::selectSubItemInDir(const QString& name) {
    this->window->selectSubItemInDir(name);
}

void FileViewer::clearContents() {
    this->getPreview<TextPreview>()->setText("");
    this->showPreview<TextPreview>();
}
