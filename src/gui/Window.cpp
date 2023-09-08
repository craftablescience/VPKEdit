#include "Window.h"

#include <cstdlib>

#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QStyleFactory>

#include <sapp/FilesystemSearchProvider.h>

#include "Config.h"
#include "EntryTree.h"
#include "FileViewer.h"

using namespace vpktool;

constexpr auto VPK_SAVE_FILTER = "Valve PacK (*.vpk);;All files (*.*)";

Window::Window(QSettings& options, QWidget* parent)
        : QMainWindow(parent) {
    this->setWindowTitle(VPKTOOL_PROJECT_NAME_PRETTY " v" VPKTOOL_PROJECT_VERSION);
    this->setWindowIcon(QIcon(":/icon.png"));
    this->setMinimumSize(900, 500);

    // File menu
    auto* fileMenu = this->menuBar()->addMenu(tr("File"));
    fileMenu->addAction(this->style()->standardIcon(QStyle::SP_FileDialogNewFolder), tr("New..."), [=] {
        this->newFile();
    });
    fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DirOpenIcon), tr("Open..."), [=] {
        this->open();
    });

    if (CFileSystemSearchProvider provider; provider.Available()) {
        auto* openRelativeToMenu = fileMenu->addMenu(this->style()->standardIcon(QStyle::SP_DirLinkIcon), tr("Open In..."));

        QList<std::tuple<QString, QString, QDir>> sourceGames;
        auto installedSteamAppCount = provider.GetNumInstalledApps();
        std::unique_ptr<uint32_t[]> steamAppIDs(provider.GetInstalledAppsEX());

        for (int i = 0; i < installedSteamAppCount; i++) {
            if (!(provider.BIsSourceGame(steamAppIDs[i]) || provider.BIsSource2Game(steamAppIDs[i])))
                continue;

            std::unique_ptr<CFileSystemSearchProvider::Game> steamGameInfo(provider.GetAppInstallDirEX(steamAppIDs[i]));
            auto relativeDirectoryPath = QDir(QString(steamGameInfo->library) + QDir::separator() + "common" + QDir::separator() + steamGameInfo->installDir);

            // Having an & before a character makes that the shortcut character and hides the &, so we need to escape it for s&box
            QString gameName(steamGameInfo->gameName);
            gameName.replace("&", "&&");
            sourceGames.emplace_back(gameName, steamGameInfo->icon, relativeDirectoryPath);
        }
        std::sort(sourceGames.begin(), sourceGames.end(), [=](const auto& lhs, const auto& rhs) {
            return std::get<0>(lhs) < std::get<0>(rhs);
        });

        for (const auto& [gameName, iconPath, relativeDirectoryPath] : sourceGames) {
            const auto relativeDirectory = relativeDirectoryPath.path();
            openRelativeToMenu->addAction(QIcon(iconPath), gameName, [=] {
                this->open(relativeDirectory);
            });
        }
    }

    this->saveFileAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save"), [=] {
        this->save();
    });
    this->saveFileAction->setDisabled(true);

    this->saveAsFileAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save to..."), [=] {
        this->saveTo();
    });
    this->saveAsFileAction->setDisabled(true);

    this->closeFileAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_BrowserReload), tr("Close"), [=] {
        this->closeFile();
    });
    this->closeFileAction->setDisabled(true);

    fileMenu->addSeparator();
    fileMenu->addAction(this->style()->standardIcon(QStyle::SP_ComputerIcon), tr("Check for updates..."), [=] {
        QDesktopServices::openUrl(QUrl(VPKTOOL_PROJECT_HOMEPAGE "/releases/latest"));
    });
    fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogCancelButton), tr("Exit"), [=] {
        this->close();
    });

    // Edit menu
    auto* editMenu = this->menuBar()->addMenu(tr("Edit"));
    this->extractAllAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Extract All"), [=] {
        this->extractAll();
    });
    this->extractAllAction->setDisabled(true);

    // Options menu
    auto* optionsMenu = this->menuBar()->addMenu(tr("Options"));

    auto* themeMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_DesktopIcon), tr("Theme..."));
    auto* themeMenuGroup = new QActionGroup(this);
    themeMenuGroup->setExclusive(true);
    for (const auto& themeName : QStyleFactory::keys()) {
        auto* action = themeMenu->addAction(themeName, [=, &options] {
            QApplication::setStyle(themeName);
            options.setValue(OPT_STYLE, themeName);
        });
        action->setCheckable(true);
        if (themeName == options.value(OPT_STYLE).toString()) {
            action->setChecked(true);
        }
        themeMenuGroup->addAction(action);
    }

    optionsMenu->addSeparator();
    auto* optionStartMaximized = optionsMenu->addAction(tr("Start Maximized"), [=, &options] {
        options.setValue(OPT_START_MAXIMIZED, !options.value(OPT_START_MAXIMIZED).toBool());
    });
    optionStartMaximized->setCheckable(true);
    optionStartMaximized->setChecked(options.value(OPT_START_MAXIMIZED).toBool());

    // Help menu
    auto* helpMenu = this->menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("About"), [=] {
        QString creditsText = "# " VPKTOOL_PROJECT_NAME_PRETTY " v" VPKTOOL_PROJECT_VERSION "\n"
                              "*Created by [craftablescience](https://github.com/craftablescience)*\n<br/>\n";
        QFile creditsFile(QCoreApplication::applicationDirPath() + "/CREDITS.md");
        if (creditsFile.open(QIODevice::ReadOnly)) {
            QTextStream in(&creditsFile);
            while(!in.atEnd()) {
                creditsText += in.readLine() + '\n';
            }
            creditsFile.close();
        }

        QMessageBox about(this);
        about.setWindowTitle(tr("About"));
        about.setIconPixmap(QIcon(":/icon.png").pixmap(64, 64));
        about.setTextFormat(Qt::TextFormat::MarkdownText);
        about.setText(creditsText);
        about.exec();
    });
    helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), "About Qt", [=] {
        QMessageBox::aboutQt(this);
    });

    // Split content into two resizeable panes
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    this->setCentralWidget(splitter);

    // Left pane
    auto* leftPane = new QWidget(splitter);
    auto* leftPaneLayout = new QVBoxLayout(leftPane);
    leftPaneLayout->setContentsMargins(4, 4, 0, 0);

    this->searchBar = new QLineEdit(leftPane);
    this->searchBar->setPlaceholderText(QString("Find..."));
    connect(this->searchBar, &QLineEdit::returnPressed, this, [=] {
        this->entryTree->setSearchQuery(this->searchBar->text());
    });
    leftPaneLayout->addWidget(this->searchBar);

    this->entryTree = new EntryTree(this, leftPane);
    leftPaneLayout->addWidget(this->entryTree);

    splitter->addWidget(leftPane);

    // Right pane
    auto* rightPane = new QWidget(splitter);
    auto* rightPaneLayout = new QVBoxLayout(rightPane);
    rightPaneLayout->setContentsMargins(0, 4, 4, 0);

    this->fileViewer = new FileViewer(this, rightPane);
    rightPaneLayout->addWidget(this->fileViewer);

    splitter->addWidget(rightPane);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 20); // qt "stretch factor" can go fuck itself this is a magic number that works

    this->statusText = new QLabel(this->statusBar());
    this->statusProgressBar = new QProgressBar(this->statusBar());

    this->statusBar()->addPermanentWidget(this->statusText, 1);
    this->statusBar()->addPermanentWidget(this->statusProgressBar, 1);

    this->clearContents();

    // Load the VPK if given one through the command-line or double-clicking a file
    // An error here means shut the application down
    const auto& args = QApplication::arguments();
    if ((args.length() > 1 && args[1].endsWith(".vpk") && QFile::exists(args[1])) && !this->loadFile(args[1])) {
        exit(1);
    }
}

void Window::newFile(const QString& startPath) {
    auto path = QFileDialog::getSaveFileName(this, tr("Save new VPK"), startPath, VPK_SAVE_FILTER);
    if (path.isEmpty()) {
        return;
    }
    std::ignore = VPK::create(path.toStdString());
    this->loadFile(path);
}

void Window::open(const QString& startPath) {
    auto path = QFileDialog::getOpenFileName(this, tr("Open VPK"), startPath, VPK_SAVE_FILTER);
    if (path.isEmpty()) {
        return;
    }
    this->loadFile(path);
}

void Window::save() {
    if (!this->vpk->bake()) {
        QMessageBox::warning(this, tr("Could not save!"), tr("An error occurred while saving the file."));
    }
}

void Window::saveTo() {
    auto savePath = QFileDialog::getExistingDirectory(this, tr("Save VPK to..."));
    if (savePath.isEmpty()) {
        return;
    }
    if (!this->vpk->bake(savePath.toStdString())) {
        QMessageBox::warning(this, tr("Could not save!"), tr("An error occurred while saving the file."));
    }
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

void Window::selectDir(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths) {
    this->fileViewer->displayDir(path, subfolders, entryPaths, this->vpk.value());
}

void Window::selectSubItemInDir(const QString& name) {
    this->entryTree->selectSubItem(name);
}

void Window::extractFile(const QString& path, QString savePath) {
    auto entry = (*this->vpk).findEntry(path.toStdString());
    if (!entry) {
        QMessageBox::critical(this, tr("Error"), "Failed to find file in VPK.");
        return;
    }

    if (savePath.isEmpty()) {
        QString filter;
        if (auto index = path.lastIndexOf('.'); index >= 0) {
            auto fileExt = path.sliced(index); // ".ext"
            auto fileExtPretty = fileExt.toUpper();
            fileExtPretty.remove('.');

            filter = fileExtPretty + " (*" + fileExt + ");;All files (*.*)";
        }
        savePath = QFileDialog::getSaveFileName(this, tr("Extract as..."), path, filter);
    }
    if (savePath.isEmpty()) {
        return;
    }
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
    this->statusText->setText(' ' + tr("Ready"));
    this->statusProgressBar->hide();

    this->searchBar->setText(QString(""));
    this->searchBar->setDisabled(true);

    this->entryTree->clearContents();

    this->fileViewer->clearContents();

    this->saveFileAction->setDisabled(true);
    this->saveAsFileAction->setDisabled(true);
    this->closeFileAction->setDisabled(true);
    this->extractAllAction->setDisabled(true);
}

bool Window::loadFile(const QString& path) {
    QString fixedPath(path);
    fixedPath.replace('\\', '/');

    this->clearContents();
    this->vpk = VPK::open(fixedPath.toStdString());
    if (!this->vpk) {
        QMessageBox::critical(this, tr("Error"), "Unable to load given VPK. Please ensure you are loading a "
                                                 "\"directory\" VPK (typically ending in _dir), not a VPK that "
                                                 "ends with 3 numbers. Loading a directory VPK will allow you "
                                                 "to browse the contents of the numbered archives next to it.");
        return false;
    }

    this->statusText->hide();
    this->statusProgressBar->show();

    this->searchBar->setDisabled(false);

    this->entryTree->loadVPK(this->vpk.value(), this->statusProgressBar, [=] {
        this->saveFileAction->setDisabled(false);
        this->saveAsFileAction->setDisabled(false);
        this->closeFileAction->setDisabled(false);
        this->extractAllAction->setDisabled(false);

        this->statusText->setText(' ' + QString("Loaded \"") + path + '\"');
        this->statusText->show();
        this->statusProgressBar->hide();
    });

    return true;
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
