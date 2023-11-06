#include "FileViewer.h"

#include <filesystem>

#include <QHBoxLayout>
#include <QMessageBox>

#include "previews/DirPreview.h"
#include "previews/info/EmptyPreview.h"
#include "previews/info/ErrorPreview.h"
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

    auto* emptyPreview = newPreview<EmptyPreview>(this);
    layout->addWidget(emptyPreview);

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
    // Get extension
    std::filesystem::path helperPath(path.toLower().toStdString());
    QString extension(helperPath.has_extension() ? helperPath.extension().string().c_str() : helperPath.stem().string().c_str());

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

void FileViewer::addEntry(const vpkedit::VPK& vpk, const QString& path) {
    this->getPreview<DirPreview>()->addEntry(vpk, path);
}

void FileViewer::removeFile(const QString& path) {
    this->getPreview<DirPreview>()->removeFile(path);
}

void FileViewer::removeDir(const QString& path) {
    if (path == this->getPreview<DirPreview>()->getCurrentPath()) {
        this->hidePreview<DirPreview>();
        return;
    }
    this->getPreview<DirPreview>()->removeDir(path);
}

void FileViewer::setSearchQuery(const QString& query) {
    this->getPreview<DirPreview>()->setSearchQuery(query);
}

void FileViewer::selectSubItemInDir(const QString& name) {
    this->window->selectSubItemInDir(name);
}

void FileViewer::clearContents() {
    this->showPreview<EmptyPreview>();
}
