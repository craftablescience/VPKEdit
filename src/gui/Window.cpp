#include "Window.h"

#include <QActionGroup>
#include <QApplication>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QProgressBar>
#include <QSplitter>
#include <QStatusBar>
#include <QStyleFactory>
#include <QThread>
#include <sapp/FilesystemSearchProvider.h>
#include <vpkedit/Version.h>

#include "config/Options.h"
#include "dialogs/ControlsDialog.h"
#include "dialogs/EntryOptionsDialog.h"
#include "dialogs/NewUpdateDialog.h"
#include "dialogs/VPKPropertiesDialog.h"
#include "EntryTree.h"
#include "FileViewer.h"

using namespace vpkedit;

constexpr auto VPK_SAVE_FILTER = "Valve Pack File (*.vpk);;All files (*.*)";

Window::Window(QWidget* parent)
        : QMainWindow(parent)
		, createFromDirWorkerThread(nullptr)
		, saveWorkerThread(nullptr)
        , extractWorkerThread(nullptr)
        , modified(false) {
    this->setWindowTitle(PROJECT_TITLE.data());
    this->setWindowIcon(QIcon(":/icon.png"));
    this->setMinimumSize(900, 500);

    // File menu
    auto* fileMenu = this->menuBar()->addMenu(tr("&File"));
    this->createEmptyVPKAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), tr("&Create Empty..."), Qt::CTRL | Qt::Key_N, [this] {
        this->newVPK(false);
    });
    this->createVPKFromDirAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), tr("Create From &Folder..."), Qt::CTRL | Qt::SHIFT | Qt::Key_N, [this] {
        this->newVPK(true);
    });
    this->openVPKAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DirIcon), tr("&Open..."), Qt::CTRL | Qt::Key_O, [this] {
        this->openVPK();
    });

	this->openVPKRelativeToMenu = nullptr;
    if (CFileSystemSearchProvider provider; provider.Available()) {
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
		if (!sourceGames.empty()) {
			std::sort(sourceGames.begin(), sourceGames.end(), [](const auto& lhs, const auto& rhs) {
				return std::get<0>(lhs) < std::get<0>(rhs);
			});

			this->openVPKRelativeToMenu = fileMenu->addMenu(this->style()->standardIcon(QStyle::SP_DirLinkIcon), tr("Open &In..."));
			for (const auto& [gameName, iconPath, relativeDirectoryPath] : sourceGames) {
				const auto relativeDirectory = relativeDirectoryPath.path();
				this->openVPKRelativeToMenu->addAction(QIcon(iconPath), gameName, [this, relativeDirectory] {
					this->openVPK(relativeDirectory);
				});
			}
		}
    }

    this->saveVPKAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("&Save"), Qt::CTRL | Qt::Key_S, [this] {
        this->saveVPK();
    });
    this->saveVPKAction->setDisabled(true);

    this->saveAsVPKAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save &As..."), Qt::CTRL | Qt::SHIFT | Qt::Key_S, [this] {
        this->saveAsVPK();
    });
    this->saveAsVPKAction->setDisabled(true);

    this->closeFileAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_BrowserReload), tr("&Close"), Qt::CTRL | Qt::Key_X, [this] {
        this->closeVPK();
    });
    this->closeFileAction->setDisabled(true);

    fileMenu->addSeparator();

	this->checkForNewUpdateNetworkManager = new QNetworkAccessManager(this);
	QObject::connect(this->checkForNewUpdateNetworkManager, &QNetworkAccessManager::finished, this, &Window::checkForUpdatesReply);
    fileMenu->addAction(this->style()->standardIcon(QStyle::SP_ComputerIcon), tr("Check For &Updates..."), Qt::CTRL | Qt::Key_U, [this] {
        this->checkForNewUpdate();
    });

    fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogCancelButton), tr("&Exit"), Qt::ALT | Qt::Key_F4, [this] {
        this->close();
    });

    // Edit menu
    auto* editMenu = this->menuBar()->addMenu(tr("&Edit"));
    this->extractAllAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("&Extract All"), Qt::CTRL | Qt::Key_E, [this] {
        this->extractAll();
    });
    this->extractAllAction->setDisabled(true);

    editMenu->addSeparator();
    this->addFileAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_FileLinkIcon), tr("&Add File..."), Qt::CTRL | Qt::Key_A, [this] {
        this->addFile(true);
    });
    this->addFileAction->setDisabled(true);

    this->addDirAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_DirLinkIcon), tr("Add &Folder..."), Qt::CTRL | Qt::SHIFT | Qt::Key_A, [this] {
        this->addDir(true);
    });
    this->addDirAction->setDisabled(true);

    editMenu->addSeparator();
    this->changeVersionAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_FileDialogContentsView), tr("&Change Version..."), Qt::CTRL | Qt::ALT | Qt::Key_V, [this] {
        this->changeVPKVersion();
    });
    this->changeVersionAction->setDisabled(true);

    // Options menu
    auto* optionsMenu = this->menuBar()->addMenu(tr("&Options"));

    auto* entryListMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("&Entry List..."));
    auto* entryListMenuAutoExpandAction = entryListMenu->addAction(tr("&Open Folder When Selected"), [this] {
		Options::invert(OPT_ENTRY_LIST_AUTO_EXPAND);
        this->entryTree->setAutoExpandDirectoryOnClick(Options::get<bool>(OPT_ENTRY_LIST_AUTO_EXPAND));
    });
    entryListMenuAutoExpandAction->setCheckable(true);
    entryListMenuAutoExpandAction->setChecked(Options::get<bool>(OPT_ENTRY_LIST_AUTO_EXPAND));

	auto* entryListMenuAutoCollapseAction = entryListMenu->addAction(tr("Start &Collapsed"), [this] {
		Options::invert(OPT_ENTRY_LIST_AUTO_COLLAPSE);
		this->entryTree->setAutoExpandDirectoryOnClick(Options::get<bool>(OPT_ENTRY_LIST_AUTO_COLLAPSE));
	});
	entryListMenuAutoCollapseAction->setCheckable(true);
	entryListMenuAutoCollapseAction->setChecked(Options::get<bool>(OPT_ENTRY_LIST_AUTO_COLLAPSE));

    auto* themeMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_DesktopIcon), tr("&Theme..."));
    auto* themeMenuGroup = new QActionGroup(this);
    themeMenuGroup->setExclusive(true);
    for (const auto& themeName : QStyleFactory::keys()) {
        auto* action = themeMenu->addAction(themeName, [=] {
            QApplication::setStyle(themeName);
            Options::set(OPT_STYLE, themeName);
        });
        action->setCheckable(true);
        if (themeName == Options::get<QString>(OPT_STYLE)) {
            action->setChecked(true);
        }
        themeMenuGroup->addAction(action);
    }

    optionsMenu->addSeparator();
    auto* optionAdvancedMode = optionsMenu->addAction(tr("&Advanced File Properties"), [=] {
        Options::invert(OPT_ADVANCED_FILE_PROPS);
    });
    optionAdvancedMode->setCheckable(true);
    optionAdvancedMode->setChecked(Options::get<bool>(OPT_ADVANCED_FILE_PROPS));

    optionsMenu->addSeparator();
    auto* optionStartMaximized = optionsMenu->addAction(tr("&Start Maximized"), [=] {
        Options::invert(OPT_START_MAXIMIZED);
    });
    optionStartMaximized->setCheckable(true);
    optionStartMaximized->setChecked(Options::get<bool>(OPT_START_MAXIMIZED));

    // Help menu
    auto* helpMenu = this->menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("&About"), Qt::Key_F1, [this] {
        this->about();
    });
    helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("About &Qt"), Qt::ALT | Qt::Key_F1, [this] {
        this->aboutQt();
    });
	helpMenu->addAction(this->style()->standardIcon(QStyle::SP_FileDialogListView), tr("Controls"), Qt::Key_F2, [this] {
		this->controls();
	});

#ifdef QT_DEBUG
    // Debug menu
    auto* debugMenu = this->menuBar()->addMenu("&Debug");
    debugMenu->addAction("New Entry Dialog (File)", [this] {
        (void) EntryOptionsDialog::getEntryOptions(false, false, "test", true, 0, this);
    });
    debugMenu->addAction("New Entry Dialog (Dir)", [this] {
        (void) EntryOptionsDialog::getEntryOptions(false, true, "test", true, 0, this);
    });
    debugMenu->addAction("Edit Entry Dialog (File)", [this] {
        (void) EntryOptionsDialog::getEntryOptions(true, false, "test", true, 0, this);
    });
    debugMenu->addAction("Edit Entry Dialog (Dir)", [this] {
        (void) EntryOptionsDialog::getEntryOptions(true, true, "test", true, 0, this);
    });
    debugMenu->addAction("New Update Dialog", [this] {
        NewUpdateDialog::getNewUpdatePrompt("https://example.com", "v1.2.3", this);
    });
    debugMenu->addAction("New VPK Dialog", [this] {
        (void) VPKPropertiesDialog::getVPKProperties(false, 2, true, this);
    });
    debugMenu->addAction("Set VPK Version Dialog", [this] {
        (void) VPKPropertiesDialog::getVPKProperties(true, 2, true, this);
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
    QObject::connect(this->searchBar, &QLineEdit::editingFinished, this, [this] {
        this->entryTree->setSearchQuery(this->searchBar->text());
        this->fileViewer->setSearchQuery(this->searchBar->text());
    });
    leftPaneLayout->addWidget(this->searchBar);

    this->entryTree = new EntryTree(this, leftPane);
    this->entryTree->setAutoExpandDirectoryOnClick(Options::get<bool>(OPT_ENTRY_LIST_AUTO_EXPAND));
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
	// todo: qt stretch 20 hack
    splitter->setStretchFactor(1, 20); // qt "stretch factor" can go fuck itself this is a magic number that works

	// Automatically collapse entry tree
	if (Options::get<bool>(OPT_ENTRY_LIST_AUTO_COLLAPSE)) {
		splitter->setSizes({0, splitter->size().width()});
	}

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

void Window::newVPK(bool fromDirectory, const QString& startPath) {
    if (this->modified && this->promptUserToKeepModifications()) {
        return;
    }

	auto vpkOptions = VPKPropertiesDialog::getVPKProperties(false, 2, false, this);
	if (!vpkOptions) {
		return;
	}
	auto [version, singleFile] = *vpkOptions;

    auto dirPath = fromDirectory ? QFileDialog::getExistingDirectory(this, tr("Use This Folder"), startPath) : "";
    if (fromDirectory && dirPath.isEmpty()) {
        return;
    }

    auto vpkPath = QFileDialog::getSaveFileName(this, tr("Save New VPK"), fromDirectory ? QString(std::filesystem::path(dirPath.toStdString()).stem().string().c_str()) + (singleFile || dirPath.endsWith("_dir") ? ".vpk" : "_dir.vpk") : startPath, VPK_SAVE_FILTER);
    if (vpkPath.isEmpty()) {
        return;
    }

    if (fromDirectory) {
	    // Set up progress bar
	    this->statusText->hide();
	    this->statusProgressBar->show();
	    this->statusBar()->show();

	    // Show progress bar is busy
		this->statusProgressBar->setValue(0);
		this->statusProgressBar->setRange(0, 0);

	    this->freezeActions(true);

	    // Set up thread
	    this->createFromDirWorkerThread = new QThread(this);
	    auto* worker = new CreateFromDirVPKWorker();
	    worker->moveToThread(this->createFromDirWorkerThread);
		// Cringe compiler moment in the lambda capture list
	    QObject::connect(this->createFromDirWorkerThread, &QThread::started, worker, [worker, vpkPath, dirPath, singleFile_=singleFile, version_=version] {
		    worker->run(vpkPath.toStdString(), dirPath.toStdString(), singleFile_, {.version = version_});
	    });
	    QObject::connect(worker, &CreateFromDirVPKWorker::taskFinished, this, [this, vpkPath] {
		    // Kill thread
		    this->createFromDirWorkerThread->quit();
		    this->createFromDirWorkerThread->wait();
		    delete this->createFromDirWorkerThread;
		    this->createFromDirWorkerThread = nullptr;

			// loadVPK freezes them right away again
		    // this->freezeActions(false);
		    this->loadVPK(vpkPath);
	    });
	    this->createFromDirWorkerThread->start();
    } else {
        (void) VPK::createEmpty(vpkPath.toStdString(), {.version = version});
	    this->loadVPK(vpkPath);
    }
}

void Window::openVPK(const QString& startPath, const QString& filePath) {
	auto path = filePath;
	if (path.isEmpty()) {
		path = QFileDialog::getOpenFileName(this, tr("Open VPK"), startPath, VPK_SAVE_FILTER);
	}
    if (path.isEmpty()) {
        return;
    }
    if (!this->loadVPK(path)) {
        this->clearContents();
    }
}

void Window::saveVPK(bool saveAs) {
	QString savePath = "";
	if (saveAs) {
		savePath = QFileDialog::getExistingDirectory(this, tr("Save VPK to..."));
		if (savePath.isEmpty()) {
			return;
		}
	}

	// Set up progress bar
	this->statusText->hide();
	this->statusProgressBar->show();

	// Get progress bar maximum
	int progressBarMax = static_cast<int>(this->vpk->getEntryCount());

	// Show progress indicator
	this->statusProgressBar->setRange(0, progressBarMax);
	this->statusProgressBar->setValue(0);

	this->freezeActions(true);

	// Set up thread
	this->saveWorkerThread = new QThread(this);
	auto* worker = new SaveVPKWorker();
	worker->moveToThread(this->saveWorkerThread);
	QObject::connect(this->saveWorkerThread, &QThread::started, worker, [this, worker, savePath] {
		worker->run(this, savePath);
	});
	QObject::connect(worker, &SaveVPKWorker::progressUpdated, this, [this, progressBarMax](int value) {
		static bool alreadyShownBusy = false;
		if (progressBarMax == value) {
			// Show busy indicator if we haven't already
			if (alreadyShownBusy) {
				return;
			}
			alreadyShownBusy = true;
			this->statusProgressBar->setValue(0);
			this->statusProgressBar->setRange(0, 0);
		} else {
			alreadyShownBusy = false;
			this->statusProgressBar->setRange(0, progressBarMax);
			this->statusProgressBar->setValue(value);
		}
	});
	QObject::connect(worker, &SaveVPKWorker::taskFinished, this, [this](bool success) {
		// Kill thread
		this->saveWorkerThread->quit();
		this->saveWorkerThread->wait();
		delete this->saveWorkerThread;
		this->saveWorkerThread = nullptr;

		this->freezeActions(false);

		this->resetStatusBar();

		if (!success) {
			QMessageBox::warning(this, tr("Could not save!"),
			                     tr("An error occurred while saving changes to the VPK. Check that you have permissions to write to the file."));
		} else {
			this->markModified(false);
		}
	});
	this->saveWorkerThread->start();
}

void Window::saveAsVPK() {
    this->saveVPK(true);
}

void Window::closeVPK() {
    this->clearContents();
    this->vpk = std::nullopt;
}

void Window::checkForNewUpdate() const {
	this->checkForNewUpdateNetworkManager->get(QNetworkRequest(QUrl(QString(PROJECT_HOMEPAGE_API.data()) + "/releases/latest")));
}

void Window::changeVPKVersion() {
    auto vpkOptions = VPKPropertiesDialog::getVPKProperties(true, this->vpk->getVersion(), false, this);
    if (!vpkOptions) {
        return;
    }
    auto [version, singleFile] = *vpkOptions;
    this->vpk->setVersion(version);

	this->resetStatusBar();

    this->markModified(true);
}

void Window::checkForUpdatesReply(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, tr("Error"), tr("Error occurred checking for updates!"));
        return;
    }
    const auto parseFailure = [this] {
        QMessageBox::critical(this, tr("Error"), tr("Invalid JSON response was retrieved checking for updates!"));
    };
    QJsonDocument response = QJsonDocument::fromJson(QString(reply->readAll()).toUtf8());

	if (!response.isObject()) {
        return parseFailure();
    }
    QJsonObject release = response.object();

	if (!release.contains("html_url") || !release["html_url"].isString()) {
		return parseFailure();
	}
	auto url = release["html_url"].toString();

	if (!release.contains("tag_name") || !release["tag_name"].isString()) {
		return parseFailure();
	}
	auto versionTag = release["tag_name"].toString();

	if (!release.contains("name") || !release["name"].isString()) {
		return parseFailure();
	}
	auto versionName = release["name"].toString();

	if (versionTag == QString("v") + PROJECT_VERSION.data()) {
		QMessageBox::information(this, tr("No New Updates"), tr("You are using the latest version of the software."));
		return;
	}
	NewUpdateDialog::getNewUpdatePrompt(url, versionName, this);
}

void Window::addFile(bool showOptions, const QString& startDir, const QString& filePath) {
	auto filepath = filePath;
	if (filepath.isEmpty()) {
		filepath = QFileDialog::getOpenFileName(this, tr("Open File"));
	}
    if (filepath.isEmpty()) {
        return;
    }

    auto prefilledPath = startDir;
    if (!prefilledPath.isEmpty()) {
        prefilledPath += '/';
    }
    prefilledPath += std::filesystem::path(filepath.toStdString()).filename().string().c_str();

	QString entryPath = prefilledPath.toLower();
	bool useArchiveVPK = false;
	int preloadBytes = 0;

	if (showOptions || Options::get<bool>(OPT_ADVANCED_FILE_PROPS)) {
		auto newEntryOptions = EntryOptionsDialog::getEntryOptions(false, false, prefilledPath, false, 0, this);
		if (!newEntryOptions) {
			return;
		}
		entryPath = std::get<0>(*newEntryOptions);
		useArchiveVPK = std::get<1>(*newEntryOptions);
		preloadBytes = std::get<2>(*newEntryOptions);
	}

	this->vpk->removeEntry(entryPath.toStdString());
	this->vpk->addEntry(entryPath.toStdString(), filepath.toStdString(), !useArchiveVPK, preloadBytes);
	this->entryTree->addEntry(entryPath);
	this->fileViewer->addEntry(this->vpk.value(), entryPath);
	this->markModified(true);
}

void Window::addDir(bool showOptions, const QString& startDir, const QString& dirPath) {
	auto dirpath = dirPath;
	if (dirpath.isEmpty()) {
		dirpath = QFileDialog::getExistingDirectory(this, tr("Open Folder"));
	}
    if (dirpath.isEmpty()) {
        return;
    }

    auto prefilledPath = startDir;
    if (!prefilledPath.isEmpty()) {
        prefilledPath += '/';
    }
    prefilledPath += std::filesystem::path(dirpath.toStdString()).filename().string().c_str();

	QString parentEntryPath = prefilledPath.toLower();
	bool useArchiveVPK = false;
	int preloadBytes = 0;

	if (showOptions || Options::get<bool>(OPT_ADVANCED_FILE_PROPS)) {
		auto newEntryOptions = EntryOptionsDialog::getEntryOptions(false, true, prefilledPath, false, 0, this);
		if (!newEntryOptions) {
			return;
		}
		parentEntryPath = std::get<0>(*newEntryOptions);
		useArchiveVPK = std::get<1>(*newEntryOptions);
		preloadBytes = std::get<2>(*newEntryOptions);
	}

    QDirIterator it(dirpath, QDir::Files | QDir::Readable, QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString subEntryPathFS = it.next().toLower();
        QString subEntryPath = parentEntryPath + subEntryPathFS.sliced(dirpath.length());
	    this->vpk->removeEntry(subEntryPath.toStdString());
        this->vpk->addEntry(subEntryPath.toStdString(), subEntryPathFS.toStdString(), !useArchiveVPK, preloadBytes);
        this->entryTree->addEntry(subEntryPath);
        this->fileViewer->addEntry(this->vpk.value(), subEntryPath);
    }
    this->markModified(true);
}

bool Window::removeFile(const QString& path) {
    if (!this->vpk->removeEntry(path.toStdString())) {
        QMessageBox::critical(this, tr("Error Removing File"), tr("There was an error removing the file at \"%1\"!").arg(path));
        return false;
    }
    this->fileViewer->removeFile(path);
    this->markModified(true);
    return true;
}

void Window::removeDir(const QString& path) const {
    this->fileViewer->removeDir(path);
}

void Window::requestEntryRemoval(const QString& path) const {
    this->entryTree->removeEntryByPath(path);
}

void Window::editFile(const QString& oldPath) {
    // Get file information and data
    auto entry = this->vpk->findEntry(oldPath.toStdString());
    if (!entry) {
        QMessageBox::critical(this, tr("Error"), tr("Unable to edit file at \"%1\": could not find file!").arg(oldPath));
        return;
    }
    auto data = this->vpk->readBinaryEntry(*entry);
    if (!data) {
        QMessageBox::critical(this, tr("Error"), tr("Unable to edit file at \"%1\": could not read file data!").arg(oldPath));
        return;
    }

    // Get new properties
    const auto options = EntryOptionsDialog::getEntryOptions(true, false, oldPath, entry->archiveIndex != VPK_DIR_INDEX, static_cast<int>(entry->preloadedData.size()), this);
    if (!options) {
        return;
    }
    const auto [newPath, useArchiveDir, preloadedBytes] = *options;

    // Remove file
    this->requestEntryRemoval(oldPath);

    // Add new file with the same info and data at the new path
    this->vpk->addBinaryEntry(newPath.toStdString(), std::move(data.value()), !useArchiveDir, preloadedBytes);
	this->entryTree->addEntry(newPath);
	this->fileViewer->addEntry(this->vpk.value(), newPath);
	this->markModified(true);
}

void Window::renameDir(const QString& oldPath, const QString& newPath_) {
	// Get new path
	QString newPath = newPath_;
	if (newPath.isEmpty()) {
		bool ok;
		newPath = QInputDialog::getText(this, tr("Rename Folder"), tr("The new path:"), QLineEdit::Normal, oldPath, &ok);
		if (!ok || newPath.isEmpty()) {
			return;
		}
	}

	std::vector<QString> paths;
	for (const auto& [directory, entries] : this->vpk->getBakedEntries()) {
		if (QString(directory.c_str()).startsWith(oldPath)) {
			for (const auto& entry : entries) {
				paths.push_back(QString(directory.c_str()) + '/' + entry.filename.c_str());
			}
		}
	}
	for (const auto& [directory, entries] : this->vpk->getUnbakedEntries()) {
		if (QString(directory.c_str()).startsWith(oldPath)) {
			for (const auto& entry : entries) {
				paths.push_back(QString(directory.c_str()) + '/' + entry.filename.c_str());
			}
		}
	}

	for (const auto& path : paths) {
		// Get data
		auto entry = this->vpk->findEntry(path.toStdString());
		if (!entry) {
			continue;
		}
		auto entryData = this->vpk->readBinaryEntry(*entry);
		if (!entryData) {
			continue;
		}

		// Remove file
		this->requestEntryRemoval(path);

		// Calculate new path
		QString newEntryPath = newPath + path.sliced(oldPath.length());

		// Add new file with the same info and data at the new path
		this->vpk->addBinaryEntry(newEntryPath.toStdString(), std::move(entryData.value()), entry->archiveIndex == VPK_DIR_INDEX, static_cast<int>(entry->preloadedData.size()));
		this->entryTree->addEntry(newEntryPath);
		this->fileViewer->addEntry(this->vpk.value(), newEntryPath);
	}
	this->markModified(true);
}

void Window::about() {
    QString creditsText = tr("# %1\n*Created by [craftablescience](https://github.com/craftablescience)*\n<br/>\n")
			.arg(PROJECT_TITLE.data());
    QFile creditsFile(":/CREDITS.md");
    if (creditsFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&creditsFile);
		creditsText += in.readAll();
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

void Window::controls() {
	ControlsDialog::showControlsDialog(this);
}

std::optional<std::vector<std::byte>> Window::readBinaryEntry(const QString& path) const {
    auto entry = this->vpk->findEntry(path.toStdString());
    if (!entry) {
        return std::nullopt;
    }
    return this->vpk->readBinaryEntry(*entry);
}

std::optional<QString> Window::readTextEntry(const QString& path) const {
    auto entry = this->vpk->findEntry(path.toStdString());
    if (!entry) {
        return std::nullopt;
    }
    auto textData = this->vpk->readTextEntry(*entry);
    if (!textData) {
        return std::nullopt;
    }
    return QString(textData->c_str());
}

void Window::selectEntryInEntryTree(const QString& path) const {
	this->entryTree->selectEntry(path);
}

void Window::selectEntryInFileViewer(const QString& path) const {
    this->fileViewer->displayEntry(path, this->vpk.value());
}

void Window::selectDirInFileViewer(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths) const {
    this->fileViewer->displayDir(path, subfolders, entryPaths, this->vpk.value());
}

bool Window::hasEntry(const QString& path) const {
	return this->entryTree->hasEntry(path);
}

void Window::selectSubItemInDir(const QString& path) const {
    this->entryTree->selectSubItem(path);
}

void Window::extractFile(const QString& path, QString savePath) {
    auto entry = this->vpk->findEntry(path.toStdString());
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
    // Set up progress bar
    this->statusText->hide();
    this->statusProgressBar->show();

    // Get progress bar maximum
    int progressBarMax = 0;
    for (const auto& [directory, entries] : this->vpk->getBakedEntries()) {
        if (predicate(QString(directory.c_str()))) {
	        progressBarMax += static_cast<int>(entries.size());
        }
    }
	for (const auto& [directory, entries] : this->vpk->getUnbakedEntries()) {
		if (predicate(QString(directory.c_str()))) {
			progressBarMax += static_cast<int>(entries.size());
		}
	}

    this->statusProgressBar->setRange(0, progressBarMax);
    this->statusProgressBar->setValue(0);

    this->freezeActions(true);

    // Set up thread
    this->extractWorkerThread = new QThread(this);
    auto* worker = new ExtractVPKWorker();
    worker->moveToThread(this->extractWorkerThread);
    QObject::connect(this->extractWorkerThread, &QThread::started, worker, [this, worker, saveDir, predicate] {
        worker->run(this, saveDir, predicate);
    });
    QObject::connect(worker, &ExtractVPKWorker::progressUpdated, this, [this](int value) {
        this->statusProgressBar->setValue(value);
    });
    QObject::connect(worker, &ExtractVPKWorker::taskFinished, this, [this] {
        // Kill thread
        this->extractWorkerThread->quit();
        this->extractWorkerThread->wait();
        delete this->extractWorkerThread;
        this->extractWorkerThread = nullptr;

        this->freezeActions(false);

        this->resetStatusBar();
    });
    this->extractWorkerThread->start();
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
    saveDir += this->vpk->getRealFilename().c_str();

    this->extractFilesIf(saveDir, [](const QString&) { return true; });
}

void Window::markModified(bool modified_) {
    this->modified = modified_;

    if (this->modified) {
        this->setWindowTitle(PROJECT_TITLE.data() + QString(" (*)"));
    } else {
        this->setWindowTitle(PROJECT_TITLE.data());
    }

    this->saveVPKAction->setDisabled(!this->modified);
}

bool Window::promptUserToKeepModifications() {
    auto response = QMessageBox::warning(this,
            tr("Save changes?"),
            tr("This VPK has unsaved changes! Would you like to save these changes first?"),
            QMessageBox::Ok | QMessageBox::Discard | QMessageBox::Cancel);
    switch (response) {
        case QMessageBox::Cancel:
            return true;
        case QMessageBox::Discard:
            return false;
        case QMessageBox::Ok:
            this->saveVPK();
            return false;
        default:
            break;
    }
    return true;
}

void Window::clearContents() {
    if (this->modified && this->promptUserToKeepModifications()) {
        return;
    }

    this->statusText->clear();
    this->statusProgressBar->hide();
    this->statusBar()->hide();

    this->searchBar->clear();
    this->searchBar->setDisabled(true);

    this->entryTree->clearContents();
    this->entryTree->setDisabled(true);

    this->fileViewer->clearContents(true);

    this->markModified(false);
    this->freezeActions(true, false); // Leave create/open unfrozen
}

void Window::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::BackButton) {
		this->fileViewer->requestNavigateBack();
		event->accept();
	} else if (event->button() == Qt::ForwardButton) {
		this->fileViewer->requestNavigateNext();
		event->accept();
	}
}

void Window::dragEnterEvent(QDragEnterEvent* event) {
	if (event->mimeData()->hasUrls() && this->fileViewer->isDirPreviewVisible()) {
		event->acceptProposedAction();
	}
}

void Window::dropEvent(QDropEvent* event) {
	if (!event->mimeData()->hasUrls() || !this->fileViewer->isDirPreviewVisible()) {
		return;
	}
	for(const QUrl& url : event->mimeData()->urls()) {
		QFileInfo info(url.toLocalFile());
		if (!info.exists()) {
			continue;
		}
		const auto& relativePath = this->fileViewer->getDirPreviewCurrentPath();
		if (info.isFile()) {
			this->addFile(false, relativePath, info.absoluteFilePath());
		} else {
			this->addDir(false, relativePath, info.absoluteFilePath());
		}
	}
}

void Window::closeEvent(QCloseEvent* event) {
    if (this->modified && this->promptUserToKeepModifications()) {
        event->ignore();
        return;
    }
    event->accept();
}

void Window::freezeActions(bool freeze, bool freezeCreationActions) const {
    this->createEmptyVPKAction->setDisabled(freeze && freezeCreationActions);
    this->createVPKFromDirAction->setDisabled(freeze && freezeCreationActions);
    this->openVPKAction->setDisabled(freeze && freezeCreationActions);
    if (this->openVPKRelativeToMenu) this->openVPKRelativeToMenu->setDisabled(freeze && freezeCreationActions);
    this->saveVPKAction->setDisabled(freeze || !this->modified);
    this->saveAsVPKAction->setDisabled(freeze);
    this->closeFileAction->setDisabled(freeze);
    this->extractAllAction->setDisabled(freeze);
    this->addFileAction->setDisabled(freeze);
    this->addDirAction->setDisabled(freeze);
    this->changeVersionAction->setDisabled(freeze);

    this->searchBar->setDisabled(freeze);
    this->entryTree->setDisabled(freeze);
    this->fileViewer->setDisabled(freeze);
}

bool Window::loadVPK(const QString& path) {
    QString fixedPath = QDir(path).absolutePath();
    fixedPath.replace('\\', '/');

    this->clearContents();
    this->freezeActions(true);

    this->vpk = VPK::open(fixedPath.toStdString());
    if (!this->vpk) {
        QMessageBox::critical(this, tr("Error"), tr(
                "Unable to load given VPK. Please ensure you are loading a "
                "\"directory\" VPK (typically ending in _dir), not a VPK that "
                "ends with 3 numbers. Loading a directory VPK will allow you "
                "to browse the contents of the numbered archives next to it.\n"
                "Also, please ensure that a game or another application is not using the VPK."));
        return false;
    }

    this->statusText->hide();
    this->statusProgressBar->show();
    this->statusBar()->show();

    this->entryTree->loadVPK(this->vpk.value(), this->statusProgressBar, [this, path] {
        this->freezeActions(false);

		this->resetStatusBar();
    });

    return true;
}

void Window::writeEntryToFile(const QString& path, const VPKEntry& entry) {
    auto data = this->vpk->readBinaryEntry(entry);
    if (!data) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to read data from the VPK for \"%1\". Please ensure that a game or another application is not using the VPK.").arg(entry.filename.c_str()));
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to write to file at \"%1\".").arg(path));
        return;
    }
    auto bytesWritten = file.write(reinterpret_cast<const char*>(data->data()), entry.length);
    if (bytesWritten != entry.length) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to write to file at \"%1\".").arg(path));
    }
    file.close();
}

void Window::resetStatusBar() {
	const auto version = this->vpk->getVersion();
	this->statusText->setText(tr(" Loaded \"%1.vpk\" - Version v%2").arg(this->vpk->getRealFilename().data()).arg(version));
	this->statusText->show();
	this->statusProgressBar->hide();
}

void CreateFromDirVPKWorker::run(const std::string& vpkPath, const std::string& contentPath, bool saveToDir, VPKOptions options) {
	(void) VPK::createFromDirectory(vpkPath, contentPath, saveToDir, options);
	emit taskFinished();
}

void SaveVPKWorker::run(Window* window, const QString& savePath) {
	int currentEntry = 0;
	bool success = window->vpk->bake(savePath.toStdString(), [this, &currentEntry](const std::string&, const VPKEntry&) {
		emit progressUpdated(++currentEntry);
	});
	emit taskFinished(success);
}

void ExtractVPKWorker::run(Window* window, const QString& saveDir, const std::function<bool(const QString&)>& predicate) {
    int currentEntry = 0;
    for (const auto& [directory, entries] : window->vpk->getBakedEntries()) {
        QString dir(directory.c_str());
        if (!predicate(dir)) {
            continue;
        }

        QDir qDir;
        if (!qDir.mkpath(saveDir + QDir::separator() + dir)) {
            QMessageBox::critical(window, tr("Error"), "Failed to create directory.");
            return;
        }

        for (const auto& entry : entries) {
            auto filePath = saveDir + QDir::separator() + dir + QDir::separator() + entry.filename.c_str();
            window->writeEntryToFile(filePath, entry);
            emit progressUpdated(++currentEntry);
        }
    }
	for (const auto& [directory, entries] : window->vpk->getUnbakedEntries()) {
		QString dir(directory.c_str());
		if (!predicate(dir)) {
			continue;
		}

		QDir qDir;
		if (!qDir.mkpath(saveDir + QDir::separator() + dir)) {
			QMessageBox::critical(window, tr("Error"), "Failed to create directory.");
			return;
		}

		for (const auto& entry : entries) {
			auto filePath = saveDir + QDir::separator() + dir + QDir::separator() + entry.filename.c_str();
			window->writeEntryToFile(filePath, entry);
			emit progressUpdated(++currentEntry);
		}
	}
    emit taskFinished();
}
