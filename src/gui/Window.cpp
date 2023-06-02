#include "Window.h"

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStyle>

#include "Config.h"
#include "EntryTree.h"
#include "FileViewer.h"

using namespace vpktool;

Window::Window(QWidget* parent)
        : QMainWindow(parent) {
    this->setWindowTitle("VPKTool v" VPKTOOL_PROJECT_VERSION);
    this->setWindowIcon(QIcon(":/icon.png"));
    this->setMinimumSize(800, 500);

    // File menu
    auto* fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(style()->standardIcon(QStyle::SP_DirOpenIcon), tr("Open"), [=] {
        this->open();
    });
    this->closeFileAction = fileMenu->addAction(style()->standardIcon(QStyle::SP_BrowserReload), tr("Close"), [=] {
        this->closeFile();
    });
    this->closeFileAction->setDisabled(true);
    fileMenu->addSeparator();
    fileMenu->addAction(style()->standardIcon(QStyle::SP_DialogCancelButton), tr("Exit"), [=] {
        this->close();
    });

    // Edit menu
    auto* editMenu = menuBar()->addMenu(tr("Edit"));
    this->extractAllAction = editMenu->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Extract All"), [=] {
        this->extractAll();
    });
    this->extractAllAction->setDisabled(true);

    // Help menu
    auto* helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(style()->standardIcon(QStyle::SP_DialogHelpButton), tr("About"), [=] {
        QMessageBox::about(this, tr("About"),
                           "VPKTool created by craftablescience\n\n"
                           "Uses VTFLib by Neil 'Jed' Jedrzejewski & Ryan Gregg, "
                           "modified by Joshua Ashton and Strata Source Contributors");
    });
    helpMenu->addAction(style()->standardIcon(QStyle::SP_DialogHelpButton), "About Qt", [] {
        qApp->aboutQt(); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    });

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    this->setCentralWidget(splitter);

    auto* hbox = new QHBoxLayout(splitter);

    this->entryTree = new EntryTree(this, splitter);
    hbox->addWidget(this->entryTree);

    this->fileViewer = new FileViewer(this, splitter);
    hbox->addWidget(this->fileViewer);

    splitter->setStretchFactor(1, 2);

    const auto& args = QApplication::arguments();
    if (args.length() > 1 && args[1].endsWith(".vpk") && QFile::exists(args[1])) {
        this->open(args[1]);
    }
}

void Window::open() {
    auto path = QFileDialog::getOpenFileName(this, tr("Open VPK"), QString(), "Valve PacK (*.vpk);;All files (*.*)");
    if (path.isEmpty()) {
        return;
    }
    this->open(path);
}

void Window::open(const QString& path) {
    this->clearContents();
    this->vpk = VPK::open(path.toStdString());
    if (!this->vpk) {
        QMessageBox::critical(this, tr("Error"), "Unable to load given VPK. Please ensure you are loading a "
                                                 "\"directory\" VPK (typically ending in _dir), not a VPK that "
                                                 "ends with 3 numbers. Loading a directory VPK will allow you "
                                                 "to browse the contents of the numbered archives next to it.");
        return;
    }

    this->closeFileAction->setDisabled(false);
    this->extractAllAction->setDisabled(false);
    this->entryTree->loadVPK(this->vpk.value());
    this->fileViewer->displayDir(QString());
}

void Window::closeFile() {
    this->clearContents();
    this->vpk = std::nullopt;
}

std::vector<std::byte> Window::readBinaryEntry(const QString& path) {
    auto entry = (*this->vpk).findEntry(path.toStdString());
    if (!entry) {
        return {};
    }
    return (*this->vpk).readBinaryEntry(*entry);
}

QString Window::readTextEntry(const QString& path) {
    auto entry = (*this->vpk).findEntry(path.toStdString());
    if (!entry) {
        return {};
    }
    return {(*this->vpk).readTextEntry(*entry).c_str()};
}

void Window::selectEntry(const QString& path) {
    this->fileViewer->displayEntry(path);
}

void Window::extractFile(const QString& path, QString savePath) {
    auto entry = (*this->vpk).findEntry(path.toStdString());
    if (!entry) {
        QMessageBox::critical(this, tr("Error"), "Failed to find file in VPK.");
        return;
    }

    if (savePath.isEmpty()) {
        savePath = QFileDialog::getExistingDirectory(this, tr("Save to..."));
    }
    if (savePath.isEmpty()) {
        return;
    }
    savePath += '/';
    savePath += entry->filename;

    this->writeEntryToFile(savePath, *entry);
}

void Window::extractFilesIf(const QString& saveDir, const std::function<bool(const QString&)>& predicate) {
    for (const auto& [directory, entries] : (*this->vpk).getEntries()) {
        QString dir(directory.c_str());
        if (!predicate(dir)) {
            continue;
        }

        QDir qDir;
        if (!qDir.mkpath(saveDir + '/' + dir)) {
            QMessageBox::critical(this, tr("Error"), "Failed to create directory.");
            return;
        }

        for (const auto& entry : entries) {
            auto filePath = saveDir + '/' + dir + '/' + entry.filename.c_str();
            this->writeEntryToFile(filePath, entry);
        }
    }
}

void Window::extractDir(const QString& path, QString saveDir) {
    if (saveDir.isEmpty()) {
        saveDir = QFileDialog::getExistingDirectory(this, tr("Extract to..."));
    }
    if (saveDir.isEmpty()) {
        return;
    }
    this->extractFilesIf(saveDir, [path](const QString& dir) { return dir.startsWith(path); });
}

void Window::extractAll(QString saveDir) {
    if (saveDir.isEmpty()) {
        saveDir = QFileDialog::getExistingDirectory(this, tr("Extract to..."));
    }
    if (saveDir.isEmpty()) {
        return;
    }
    saveDir += '/';
    saveDir += (*this->vpk).getPrettyFileName();

    this->extractFilesIf(saveDir, [](const QString&) { return true; });
}

void Window::clearContents() {
    this->entryTree->clearContents();
    this->fileViewer->clearContents();
    this->closeFileAction->setDisabled(true);
    this->extractAllAction->setDisabled(true);
}

void Window::writeEntryToFile(const QString& path, const VPKEntry& entry) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        auto data = (*this->vpk).readBinaryEntry(entry);
        auto bytesWritten = file.write(reinterpret_cast<const char*>(data.data()), entry.length);
        if (bytesWritten != entry.length) {
            QMessageBox::critical(this, tr("Error"), QString("Failed to write to file at \"") + path + "\".");
            return;
        }
        file.close();
    } else {
        QMessageBox::critical(this, tr("Error"), QString("Failed to write to file at \"") + path + "\".");
        return;
    }
}
