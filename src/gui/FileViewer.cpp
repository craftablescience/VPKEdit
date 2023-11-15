#include "FileViewer.h"

#include <filesystem>

#include <QHBoxLayout>

#include "previews/DirPreview.h"
#include "previews/info/EmptyPreview.h"
#include "previews/info/FileLoadErrorPreview.h"
#include "previews/info/InvalidMDLErrorPreview.h"
#include "previews/ImagePreview.h"
#include "previews/MDLPreview.h"
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

    auto* fileLoadErrorPreview = newPreview<FileLoadErrorPreview>(this);
    layout->addWidget(fileLoadErrorPreview);

    auto* imagePreview = newPreview<ImagePreview>(this);
    layout->addWidget(imagePreview);

	auto* invalidMDLErrorPreview = newPreview<InvalidMDLErrorPreview>(this);
	layout->addWidget(invalidMDLErrorPreview);

	auto* mdlPreview = newPreview<MDLPreview>(this, this);
	layout->addWidget(mdlPreview);

    auto* textPreview = newPreview<TextPreview>(this);
    layout->addWidget(textPreview);

    auto* vtfPreview = newPreview<VTFPreview>(this);
    layout->addWidget(vtfPreview);

    this->clearContents();
}

void FileViewer::displayEntry(const QString& path, const VPK& vpk) {
    // Get extension
    std::filesystem::path helperPath(path.toLower().toStdString());
    QString extension(helperPath.has_extension() ? helperPath.extension().string().c_str() : helperPath.stem().string().c_str());

    this->clearContents();
    if (ImagePreview::EXTENSIONS.contains(extension)) {
	    // Image
	    auto binary = this->window->readBinaryEntry(path);
	    if (!binary) {
		    this->showPreview<FileLoadErrorPreview>();
		    return;
	    }
	    this->showPreview<ImagePreview>();
	    this->getPreview<ImagePreview>()->setData(*binary);
    } else if (MDLPreview::EXTENSIONS.contains(extension)) {
		// MDL (model)
	    auto binary = this->window->readBinaryEntry(path);
	    if (!binary) {
		    this->showPreview<FileLoadErrorPreview>();
		    return;
	    }
	    this->showPreview<MDLPreview>();
	    this->getPreview<MDLPreview>()->setMesh(path, vpk);
    } else if (VTFPreview::EXTENSIONS.contains(extension)) {
        // VTF (texture)
        auto binary = this->window->readBinaryEntry(path);
        if (!binary) {
            this->showPreview<FileLoadErrorPreview>();
            return;
        }
	    this->showPreview<VTFPreview>();
        this->getPreview<VTFPreview>()->setData(*binary);
    } else if (TextPreview::EXTENSIONS.contains(extension)) {
        // Text
        auto text = this->window->readTextEntry(path);
        if (!text) {
            this->showPreview<FileLoadErrorPreview>();
            return;
        }
	    this->showPreview<TextPreview>();
        this->getPreview<TextPreview>()->setText(*text, extension);
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

void FileViewer::selectSubItemInDir(const QString& name) const {
    this->window->selectSubItemInDir(name);
}

bool FileViewer::isDirPreviewVisible() {
	return this->getPreview<DirPreview>()->isVisible();
}

const QString& FileViewer::getDirPreviewCurrentPath() {
	return this->getPreview<DirPreview>()->getCurrentPath();
}

void FileViewer::clearContents() {
    this->showPreview<EmptyPreview>();
}
