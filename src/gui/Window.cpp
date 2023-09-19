#include "Window.h"

#include <cstdlib>
#include <optional>

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
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

#include "popups/NewEntryDialog.h"
#include "popups/NewVPKDialog.h"
#include "Config.h"
#include "EntryTree.h"
#include "FileViewer.h"
#include "Options.h"

using namespace vpkedit;

constexpr auto VPK_SAVE_FILTER = "Valve PacK (*.vpk);;All files (*.*)";

Window::Window(QSettings& options, QWidget* parent)
        : QMainWindow(parent)
        , modified(false) {
    this->setWindowIcon(QIcon(":/icon.png"));
    this->setMinimumSize(900, 500);

    // File menu
    auto* fileMenu = this->menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), tr("&New..."), Qt::CTRL | Qt::Key_N, [=] {
        this->newVPK();
    });
    fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DirIcon), tr("&Open..."), Qt::CTRL | Qt::Key_O, [=] {
        this->openVPK();
    });

    if (CFileSystemSearchProvider provider; provider.Available()) {
        auto* openRelativeToMenu = fileMenu->addMenu(this->style()->standardIcon(QStyle::SP_DirLinkIcon), tr("Open &In..."));

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
                this->openVPK(relativeDirectory);
            });
        }
    }

    this->saveVPKAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("&Save"), Qt::CTRL | Qt::Key_S, [=] {
        this->saveVPK();
    });
    this->saveVPKAction->setDisabled(true);

    this->saveAsVPKAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save &As..."), Qt::CTRL | Qt::SHIFT | Qt::Key_S, [=] {
        this->saveAsVPK();
    });
    this->saveAsVPKAction->setDisabled(true);

    this->closeFileAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_BrowserReload), tr("&Close"), Qt::CTRL | Qt::Key_X, [=] {
        this->closeVPK();
    });
    this->closeFileAction->setDisabled(true);

    fileMenu->addSeparator();
    fileMenu->addAction(this->style()->standardIcon(QStyle::SP_ComputerIcon), tr("Check For &Updates..."), [=] {
        Window::checkForUpdates();
    });
    fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogCancelButton), tr("&Exit"), Qt::ALT | Qt::Key_F4, [=] {
        this->close();
    });

    // Edit menu
    auto* editMenu = this->menuBar()->addMenu(tr("&Edit"));
    this->addFileAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_FileLinkIcon), tr("&Add File..."), Qt::CTRL | Qt::Key_A, [=] {
        this->addFile();
    });
    this->addFileAction->setDisabled(true);

    editMenu->addSeparator();
    this->extractAllAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("&Extract All"), Qt::CTRL | Qt::Key_E, [=] {
        this->extractAll();
    });
    this->extractAllAction->setDisabled(true);

    // Options menu
    auto* optionsMenu = this->menuBar()->addMenu(tr("&Options"));

    auto* entryListMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("&Entry List..."));
    auto* entryListMenuAutoExpandAction = entryListMenu->addAction(tr("&Open Folder When Selected"), [=, &options] {
        const bool newValue = !options.value(OPT_ENTRY_LIST_AUTO_EXPAND).toBool();
        this->entryTree->setAutoExpandDirectoryOnClick(newValue);
        options.setValue(OPT_ENTRY_LIST_AUTO_EXPAND, newValue);
    });
    entryListMenuAutoExpandAction->setCheckable(true);
    entryListMenuAutoExpandAction->setChecked(options.value(OPT_ENTRY_LIST_AUTO_EXPAND).toBool());

    auto* themeMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_DesktopIcon), tr("&Theme..."));
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
    auto* optionAdvancedMode = optionsMenu->addAction(tr("&Advanced Mode"), [=, &options] {
        options.setValue(OPT_ADV_MODE, !options.value(OPT_ADV_MODE).toBool());
    });
    optionAdvancedMode->setCheckable(true);
    optionAdvancedMode->setChecked(options.value(OPT_ADV_MODE).toBool());

    optionsMenu->addSeparator();
    auto* optionStartMaximized = optionsMenu->addAction(tr("&Start Maximized"), [=, &options] {
        options.setValue(OPT_START_MAXIMIZED, !options.value(OPT_START_MAXIMIZED).toBool());
    });
    optionStartMaximized->setCheckable(true);
    optionStartMaximized->setChecked(options.value(OPT_START_MAXIMIZED).toBool());

    // Help menu
    auto* helpMenu = this->menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("&About"), Qt::Key_F1, [=] {
        this->about();
    });
    helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), "About &Qt", [=] {
        this->aboutQt();
    });

#ifdef QT_DEBUG
    // Debug menu
    auto* debugMenu = this->menuBar()->addMenu("&Debug");
    debugMenu->addAction("getNewEntryOptions", [=] {
        std::ignore = NewEntryDialog::getNewEntryOptions(this, "test");
    });
    debugMenu->addAction("getNewVPKOptions", [=] {
        std::ignore = NewVPKDialog::getNewVPKOptions(this);
    });
#endif

    // Call after the menu is created, it controls the visibility of the save button
    this->markModified(false);

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
    this->entryTree->setAutoExpandDirectoryOnClick(options.value(OPT_ENTRY_LIST_AUTO_EXPAND).toBool());
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
    if ((args.length() > 1 && args[1].endsWith(".vpk") && QFile::exists(args[1])) && !this->loadVPK(args[1])) {
        exit(1);
    }
}

void Window::newVPK(const QString& startPath) {
    if (this->modified && this->promptUserToKeepModifications()) {
        return;
    }

    auto path = QFileDialog::getSaveFileName(this, tr("Save new VPK"), startPath, VPK_SAVE_FILTER);
    if (path.isEmpty()) {
        return;
    }
    auto vpkOptions = NewVPKDialog::getNewVPKOptions(this);
    if (!vpkOptions) {
        return;
    }
    auto [version] = *vpkOptions;
    const bool cs2 = version == VPK_ID;
    if (cs2) {
        version = 2;
    }
    std::ignore = VPK::create(path.toStdString(), version, cs2);
    this->loadVPK(path);
}

void Window::openVPK(const QString& startPath) {
    auto path = QFileDialog::getOpenFileName(this, tr("Open VPK"), startPath, VPK_SAVE_FILTER);
    if (path.isEmpty()) {
        return;
    }
    this->loadVPK(path);
}

bool Window::saveVPK() {
    if (!this->vpk->bake()) {
        QMessageBox::warning(this, tr("Could not save!"),
                             tr("An error occurred while saving changes to the VPK. Check that you have permissions to write to the file."));
        return false;
    }
    this->markModified(false);
    return true;
}

bool Window::saveAsVPK() {
    auto savePath = QFileDialog::getExistingDirectory(this, tr("Save VPK to..."));
    if (savePath.isEmpty()) {
        return false;
    }
    if (!this->vpk->bake(savePath.toStdString())) {
        QMessageBox::warning(this, tr("Could not save!"),
                             tr("An error occurred while saving the VPK. Check that you have permissions to write to the given location."));
        return false;
    }
    this->markModified(false);
    return true;
}

void Window::closeVPK() {
    this->clearContents();
    this->vpk = std::nullopt;
}

void Window::checkForUpdates() {
    QDesktopServices::openUrl(QUrl(VPKEDIT_PROJECT_HOMEPAGE "/releases/latest"));
}

void Window::addFile(const QString& startPath) {
    auto filepath = QFileDialog::getOpenFileName(this, tr("Open File"));
    if (filepath.isEmpty()) {
        return;
    }

    auto prefilledPath = startPath;
    if (!prefilledPath.isEmpty()) {
        prefilledPath += '/';
    }
    prefilledPath += std::filesystem::path(filepath.toStdString()).filename().string().c_str();

    auto newEntryOptions = NewEntryDialog::getNewEntryOptions(this, prefilledPath);
    if (!newEntryOptions) {
        return;
    }
    const auto [entryPath, useArchiveVPK, preloadBytes] = *newEntryOptions;
    this->vpk->addEntry(entryPath.toStdString(), filepath.toStdString(), !useArchiveVPK, preloadBytes);
    this->markModified(true);
    this->entryTree->addEntry(entryPath);
}

bool Window::removeFile(const QString& filepath) {
    if (!this->vpk->removeEntry(filepath.toStdString())) {
        QMessageBox::critical(this, tr("Error Removing File"), tr("There was an error removing the file at \"%1\"").arg(filepath));
        return false;
    }
    this->markModified(true);
    return true;
}

void Window::about() {
    QString creditsText = "# " VPKEDIT_PROJECT_NAME_PRETTY " v" VPKEDIT_PROJECT_VERSION "\n"
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
}

void Window::aboutQt() {
    QMessageBox::aboutQt(this);
}

std::optional<std::vector<std::byte>> Window::readBinaryEntry(const QString& path) {
    auto entry = (*this->vpk).findEntry(path.toStdString());
    if (!entry) {
        return std::nullopt;
    }
    return (*this->vpk).readBinaryEntry(*entry);
}

std::optional<QString> Window::readTextEntry(const QString& path) {
    auto entry = (*this->vpk).findEntry(path.toStdString());
    if (!entry) {
        return std::nullopt;
    }
    auto textData = (*this->vpk).readTextEntry(*entry);
    if (!textData) {
        return std::nullopt;
    }
    return QString(textData->c_str());
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

void Window::markModified(bool modified_) {
    this->modified = modified_;

    if (this->modified) {
        this->setWindowTitle(VPKEDIT_PROJECT_NAME_PRETTY " v" VPKEDIT_PROJECT_VERSION " (*)");
    } else {
        this->setWindowTitle(VPKEDIT_PROJECT_NAME_PRETTY " v" VPKEDIT_PROJECT_VERSION);
    }

    this->saveVPKAction->setDisabled(!this->modified);
}

bool Window::promptUserToKeepModifications() {
    auto response = QMessageBox::warning(this, tr("Save changes?"), tr("Hold up! Would you like to save changes to the VPK first?"), QMessageBox::Ok | QMessageBox::Discard | QMessageBox::Cancel);
    if (response == QMessageBox::Cancel) {
        return true;
    }
    if (response == QMessageBox::Discard) {
        return false;
    }
    if (response == QMessageBox::Ok) {
        this->saveVPK();
        return false;
    }
    return true;
}

void Window::clearContents() {
    if (this->modified && this->promptUserToKeepModifications()) {
        return;
    }

    this->statusText->setText(' ' + tr("Ready"));
    this->statusProgressBar->hide();

    this->searchBar->setText(QString(""));
    this->searchBar->setDisabled(true);

    this->entryTree->clearContents();

    this->fileViewer->clearContents();

    this->saveAsVPKAction->setDisabled(true);
    this->closeFileAction->setDisabled(true);
    this->addFileAction->setDisabled(true);
    this->extractAllAction->setDisabled(true);

    this->markModified(false);
}

void Window::closeEvent(QCloseEvent* event) {
    if (this->modified && this->promptUserToKeepModifications()) {
        event->ignore();
        return;
    }
    event->accept();
}

bool Window::loadVPK(const QString& path) {
    QString fixedPath(path);
    fixedPath.replace('\\', '/');

    this->clearContents();
    this->vpk = VPK::open(fixedPath.toStdString());
    if (!this->vpk) {
        QMessageBox::critical(this, tr("Error"), "Unable to load given VPK. Please ensure you are loading a "
                                                 "\"directory\" VPK (typically ending in _dir), not a VPK that "
                                                 "ends with 3 numbers. Loading a directory VPK will allow you "
                                                 "to browse the contents of the numbered archives next to it.\n"
                                                 "Also, please ensure that a game or another application is not using the VPK.");
        return false;
    }

    this->statusText->hide();
    this->statusProgressBar->show();

    this->searchBar->setDisabled(false);

    this->entryTree->loadVPK(this->vpk.value(), this->statusProgressBar, [=] {
        this->saveAsVPKAction->setDisabled(false);
        this->closeFileAction->setDisabled(false);
        this->addFileAction->setDisabled(false);
        this->extractAllAction->setDisabled(false);

        this->statusText->setText(' ' + QString("Loaded \"") + path + '\"');
        this->statusText->show();
        this->statusProgressBar->hide();
    });

    return true;
}

void Window::writeEntryToFile(const QString& path, const VPKEntry& entry) {
    auto data = (*this->vpk).readBinaryEntry(entry);
    if (!data) {
        QMessageBox::critical(this, tr("Error"), QString("Failed to read data from the VPK for \"") + entry.filename.c_str() + "\". Please ensure that a game or another application is not using the VPK.");
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), QString("Failed to write to file at \"") + path + "\".");
        return;
    }
    auto bytesWritten = file.write(reinterpret_cast<const char*>(data->data()), entry.length);
    if (bytesWritten != entry.length) {
        QMessageBox::critical(this, tr("Error"), QString("Failed to write to file at \"") + path + "\".");
    }
    file.close();
}
