#include "Window.h"

#include <chrono>
#include <fstream>

#include <KeyValue.h>
#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
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
#include <QStringDecoder>
#include <QStyleFactory>
#include <QThread>
#include <QTimer>
#include <SAPP/SAPP.h>
#include <vpkedit/format/VPK.h>
#ifdef VPKEDIT_ZIP_COMPRESSION
#include <vpkedit/format/ZIP.h>
#endif
#include <vpkedit/Version.h>

#include "dialogs/ControlsDialog.h"
#include "dialogs/EntryOptionsDialog.h"
#include "dialogs/NewUpdateDialog.h"
#include "dialogs/NewVPKOptionsDialog.h"
#include "dialogs/PackFileOptionsDialog.h"
#include "dialogs/VerifyChecksumsDialog.h"
#include "dialogs/VerifySignatureDialog.h"
#include "utility/DiscordPresence.h"
#include "utility/Options.h"
#include "utility/TGADecoder.h"
#include "EntryTree.h"
#include "FileViewer.h"

using namespace vpkedit;

Window::Window(QWidget* parent)
		: QMainWindow(parent)
		, modified(false)
		, dropEnabled(true) {
	this->setWindowTitle(PROJECT_TITLE.data());
	this->setWindowIcon(QIcon(":/logo.png"));
	this->setMinimumSize(900, 500);

	const auto showRestartWarning = [this] {
		static bool shownRestartMessage = false;
		if (!shownRestartMessage) {
			QMessageBox::warning(this, tr("Restart Required"), tr("The application must be restarted for these settings to take effect."));
			shownRestartMessage = true;
		}
	};

	// File menu
	auto* fileMenu = this->menuBar()->addMenu(tr("File"));
	this->createEmptyVPKAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), tr("Create Empty VPK..."), Qt::CTRL | Qt::Key_N, [this] {
		this->newVPK(false);
	});
	this->createVPKFromDirAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), tr("Create VPK From Folder..."), Qt::CTRL | Qt::SHIFT | Qt::Key_N, [this] {
		this->newVPK(true);
	});
	this->openAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DirIcon), tr("Open..."), Qt::CTRL | Qt::Key_O, [this] {
		this->openPackFile();
	});

	this->openRelativeToMenu = fileMenu->addMenu(this->style()->standardIcon(QStyle::SP_DirLinkIcon), tr("Open In..."));
	this->rebuildOpenInMenu();

	this->openRecentMenu = fileMenu->addMenu(this->style()->standardIcon(QStyle::SP_DirLinkIcon), tr("Open Recent..."));
	this->rebuildOpenRecentMenu(Options::get<QStringList>(STR_OPEN_RECENT));

	this->saveAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save"), Qt::CTRL | Qt::Key_S, [this] {
		this->savePackFile();
	});
	this->saveAction->setDisabled(true);

	this->saveAsAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Save As..."), Qt::CTRL | Qt::SHIFT | Qt::Key_S, [this] {
		this->saveAsPackFile();
	});
	this->saveAsAction->setDisabled(true);

	this->closeFileAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_BrowserReload), tr("Close"), Qt::CTRL | Qt::Key_X, [this] {
		this->closePackFile();
	});
	this->closeFileAction->setDisabled(true);

	fileMenu->addSeparator();
	fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("Donate On Ko-fi..."), [] {
		QDesktopServices::openUrl({"https://ko-fi.com/craftablescience"});
	});

	this->checkForNewUpdateNetworkManager = new QNetworkAccessManager(this);
	QObject::connect(this->checkForNewUpdateNetworkManager, &QNetworkAccessManager::finished, this, &Window::checkForUpdatesReply);

	fileMenu->addAction(this->style()->standardIcon(QStyle::SP_ComputerIcon), tr("Check For Updates..."), Qt::CTRL | Qt::Key_U, [this] {
		this->checkForNewUpdate();
	});

	fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogCancelButton), tr("Exit"), Qt::ALT | Qt::Key_F4, [this] {
		this->close();
	});

	// Edit menu
	auto* editMenu = this->menuBar()->addMenu(tr("Edit"));
	this->extractAllAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Extract All"), Qt::CTRL | Qt::Key_E, [this] {
		this->extractAll();
	});
	this->extractAllAction->setDisabled(true);

	editMenu->addSeparator();
	this->addFileAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_FileLinkIcon), tr("Add File..."), Qt::CTRL | Qt::Key_A, [this] {
		this->addFile(true);
	});
	this->addFileAction->setDisabled(true);

	this->addDirAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_DirLinkIcon), tr("Add Folder..."), Qt::CTRL | Qt::SHIFT | Qt::Key_A, [this] {
		this->addDir(true);
	});
	this->addDirAction->setDisabled(true);

	editMenu->addSeparator();
	this->setPropertiesAction = editMenu->addAction(this->style()->standardIcon(QStyle::SP_FileDialogContentsView), tr("Properties..."), Qt::CTRL | Qt::Key_P, [this] {
		this->setProperties();
	});
	this->setPropertiesAction->setDisabled(true);

	// Options menu
	auto* optionsMenu = this->menuBar()->addMenu(tr("Options"));

	auto* generalMenu = optionsMenu->addMenu(QIcon(":/logo.png"), "General...");
	auto* optionAdvancedMode = generalMenu->addAction(tr("Advanced File Properties"), [] {
		Options::invert(OPT_ADVANCED_FILE_PROPS);
	});
	optionAdvancedMode->setCheckable(true);
	optionAdvancedMode->setChecked(Options::get<bool>(OPT_ADVANCED_FILE_PROPS));

	generalMenu->addSeparator();
	auto* openInEnableAction = generalMenu->addAction(tr("Disable Open In Menu"), [this] {
		Options::invert(OPT_DISABLE_STEAM_SCANNER);
		this->rebuildOpenInMenu();
	});
	openInEnableAction->setCheckable(true);
	openInEnableAction->setChecked(Options::get<bool>(OPT_DISABLE_STEAM_SCANNER));

	auto* optionDisableStartupCheck = generalMenu->addAction(tr("Disable Startup Update Check"), [] {
		Options::invert(OPT_DISABLE_STARTUP_UPDATE_CHECK);
	});
	optionDisableStartupCheck->setCheckable(true);
	optionDisableStartupCheck->setChecked(Options::get<bool>(OPT_DISABLE_STARTUP_UPDATE_CHECK));

	generalMenu->addSeparator();
	auto* optionStartMaximized = generalMenu->addAction(tr("Start Maximized"), [] {
		Options::invert(OPT_START_MAXIMIZED);
	});
	optionStartMaximized->setCheckable(true);
	optionStartMaximized->setChecked(Options::get<bool>(OPT_START_MAXIMIZED));

	auto* languageMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("Language..."));
	auto* languageMenuGroup = new QActionGroup(languageMenu);
	languageMenuGroup->setExclusive(true);
	const QVector<QPair<QString, QString>> languageToLocaleMapping = {
			{tr("System Language"), ""},
			{"", ""}, // Separator
			{u8"Bosanski",		"bs_BA"},
			{u8"简体中文",		"zh_CN"},
			{u8"Nederlands",	"nl"},
			{u8"English",		"en"},
			{u8"日本語",			"ja"},
			{u8"Italiano",		"it"},
			{u8"Polski",		"pl"},
			{u8"Español",		"es"},
			{u8"Svenska",		"sv"},
			{u8"Русский",		"ru_RU"},
	};
	for (const auto& [language, locale] : languageToLocaleMapping) {
		if (language.isEmpty() && locale.isEmpty()) {
			languageMenu->addSeparator();
			continue;
		}
		auto* action = languageMenu->addAction(language, [showRestartWarning, locale_=locale] {
			showRestartWarning();
			Options::set(OPT_LANGUAGE_OVERRIDE, locale_);
		});
		action->setCheckable(true);
		if (locale == Options::get<QString>(OPT_LANGUAGE_OVERRIDE)) {
			action->setChecked(true);
		}
		languageMenuGroup->addAction(action);
	}

	auto* themeMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_DesktopIcon), tr("Theme..."));
	auto* themeMenuGroup = new QActionGroup(themeMenu);
	themeMenuGroup->setExclusive(true);
	for (const auto& themeName : QStyleFactory::keys()) {
		auto* action = themeMenu->addAction(themeName, [this, themeName] {
			QApplication::setStyle(themeName);
			Options::set(OPT_STYLE, themeName);
			emit this->themeUpdated();
		});
		action->setCheckable(true);
		if (themeName == Options::get<QString>(OPT_STYLE)) {
			action->setChecked(true);
		}
		themeMenuGroup->addAction(action);
	}

	// Not translating this menu name, the translation is the same everywhere
	auto* discordMenu = optionsMenu->addMenu(QIcon{":/icons/discord.png"}, "Discord...");
	const auto setupDiscordRichPresence = [] {
		auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
#ifdef _WIN32
		std::tm currentTimeVal{};
		localtime_s(&currentTimeVal, &time);
		auto* currentTime = &currentTimeVal;
#else
		auto* currentTime = std::localtime(&time);
#endif
		if (currentTime->tm_mon == 3 && currentTime->tm_mday == 1) {
			// its april 1st you know what that means
			DiscordPresence::init("1232981268472533032");
			DiscordPresence::setDetails("Customizing character...");
			DiscordPresence::setState("Ponyville Train Station");
		} else {
			DiscordPresence::init("1222285763459158056");
			DiscordPresence::setState("Editing an archive file");
			DiscordPresence::setLargeImageText(PROJECT_TITLE.data());
		}
		DiscordPresence::setLargeImage("icon");
		DiscordPresence::setStartTimestamp(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
		DiscordPresence::setTopButton({"View on GitHub", std::string{PROJECT_HOMEPAGE}});
	};
	auto* discordEnableAction = discordMenu->addAction(tr("Enable Rich Presence"), [setupDiscordRichPresence] {
		Options::invert(OPT_ENABLE_DISCORD_RICH_PRESENCE);
		if (Options::get<bool>(OPT_ENABLE_DISCORD_RICH_PRESENCE)) {
			setupDiscordRichPresence();
		} else {
			DiscordPresence::shutdown();
		}
	});
	discordEnableAction->setCheckable(true);
	discordEnableAction->setChecked(Options::get<bool>(OPT_ENABLE_DISCORD_RICH_PRESENCE));

	if (Options::get<bool>(OPT_ENABLE_DISCORD_RICH_PRESENCE)) {
		setupDiscordRichPresence();
	}
	auto* discordUpdateTimer = new QTimer(this);
	QObject::connect(discordUpdateTimer, &QTimer::timeout, this, &DiscordPresence::update);
	discordUpdateTimer->start(20);

	auto* entryListMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Entry Tree..."));
	auto* entryListMenuAutoExpandAction = entryListMenu->addAction(tr("Expand Folder When Selected"), [this] {
		Options::invert(OPT_ENTRY_TREE_AUTO_EXPAND);
		this->entryTree->setAutoExpandDirectoryOnClick(Options::get<bool>(OPT_ENTRY_TREE_AUTO_EXPAND));
	});
	entryListMenuAutoExpandAction->setCheckable(true);
	entryListMenuAutoExpandAction->setChecked(Options::get<bool>(OPT_ENTRY_TREE_AUTO_EXPAND));

	auto* entryListMenuAutoCollapseAction = entryListMenu->addAction(tr("Start Collapsed"), [this] {
		Options::invert(OPT_ENTRY_TREE_AUTO_COLLAPSE);
		this->entryTree->setAutoExpandDirectoryOnClick(Options::get<bool>(OPT_ENTRY_TREE_AUTO_COLLAPSE));
	});
	entryListMenuAutoCollapseAction->setCheckable(true);
	entryListMenuAutoCollapseAction->setChecked(Options::get<bool>(OPT_ENTRY_TREE_AUTO_COLLAPSE));

	auto* entryListMenuHideIconsAction = entryListMenu->addAction(tr("Hide Icons"), [this] {
		Options::invert(OPT_ENTRY_TREE_HIDE_ICONS);
		this->entryTree->setAutoExpandDirectoryOnClick(Options::get<bool>(OPT_ENTRY_TREE_HIDE_ICONS));
	});
	entryListMenuHideIconsAction->setCheckable(true);
	entryListMenuHideIconsAction->setChecked(Options::get<bool>(OPT_ENTRY_TREE_HIDE_ICONS));

	// Tools menu
	auto* toolsMenu = this->menuBar()->addMenu(tr("Tools"));

	this->toolsGeneralMenu = toolsMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileIcon), tr("General"));
	this->toolsGeneralMenu->addAction(this->style()->standardIcon(QStyle::SP_FileDialogContentsView), tr("Verify Checksums"), [this] {
		VerifyChecksumsDialog::showDialog(*this->packFile, this);
	});
	this->toolsGeneralMenu->addAction(this->style()->standardIcon(QStyle::SP_FileDialogContentsView), tr("Verify Signature"), [this] {
		VerifySignatureDialog::showDialog(*this->packFile, this);
	});
	this->toolsGeneralMenu->setDisabled(true);

	this->toolsVPKMenu = toolsMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileIcon), "VPK");
	this->toolsVPKMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), tr("Generate Public/Private Key Files..."), [this] {
		this->generateKeyPairFiles();
	});
	this->toolsVPKMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), tr("Sign File..."), [this] {
		this->signPackFile();
	});
	this->toolsVPKMenu->setDisabled(true);

	// Help menu
	auto* helpMenu = this->menuBar()->addMenu(tr("Help"));
	helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("About"), Qt::Key_F1, [this] {
		this->about();
	});
	helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("About Qt"), Qt::ALT | Qt::Key_F1, [this] {
		QMessageBox::aboutQt(this);
	});
	helpMenu->addAction(this->style()->standardIcon(QStyle::SP_FileDialogListView), tr("Controls"), Qt::Key_F2, [this] {
		ControlsDialog::showDialog(this);
	});

#ifdef DEBUG
	// Debug menu
	auto* debugMenu = this->menuBar()->addMenu(tr("Debug"));

	auto* debugDialogsMenu = debugMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Dialogs"));
	debugDialogsMenu->addAction("New Entry Dialog (File) [VPK]", [this] {
		(void) EntryOptionsDialog::getEntryOptions(false, false, "test", PackFileType::VPK, {}, this);
	});
	debugDialogsMenu->addAction("New Entry Dialog (Dir) [VPK]", [this] {
		(void) EntryOptionsDialog::getEntryOptions(false, true, "test", PackFileType::VPK, {}, this);
	});
	debugDialogsMenu->addAction("Edit Entry Dialog (File) [VPK]", [this] {
		(void) EntryOptionsDialog::getEntryOptions(true, false, "test", PackFileType::VPK, {}, this);
	});
	debugDialogsMenu->addAction("Edit Entry Dialog (Dir) [VPK]", [this] {
		(void) EntryOptionsDialog::getEntryOptions(true, true, "test", PackFileType::VPK, {}, this);
	});
	debugDialogsMenu->addAction("New Entry Dialog (File) [ZIP/BSP]", [this] {
		(void) EntryOptionsDialog::getEntryOptions(false, false, "test", PackFileType::ZIP, {}, this);
	});
	debugDialogsMenu->addAction("New Entry Dialog (Dir) [ZIP/BSP]", [this] {
		(void) EntryOptionsDialog::getEntryOptions(false, true, "test", PackFileType::ZIP, {}, this);
	});
	debugDialogsMenu->addAction("Edit Entry Dialog (File) [ZIP/BSP]", [this] {
		(void) EntryOptionsDialog::getEntryOptions(true, false, "test", PackFileType::ZIP, {}, this);
	});
	debugDialogsMenu->addAction("Edit Entry Dialog (Dir) [ZIP/BSP]", [this] {
		(void) EntryOptionsDialog::getEntryOptions(true, true, "test", PackFileType::ZIP, {}, this);
	});
	debugDialogsMenu->addAction("New Update Dialog", [this] {
		NewUpdateDialog::getNewUpdatePrompt("https://example.com", "v1.2.3", "sample description", this);
	});
	debugDialogsMenu->addAction("Create Empty VPK Options Dialog", [this] {
		(void) NewVPKOptionsDialog::getNewVPKOptions(false, {}, false, this);
	});
	debugDialogsMenu->addAction("Create VPK From Folder Options Dialog", [this] {
		(void) NewVPKOptionsDialog::getNewVPKOptions(true, {}, false, this);
	});
	debugDialogsMenu->addAction("PackFile Options Dialog [VPK]", [this] {
		(void) PackFileOptionsDialog::getPackFileOptions(PackFileType::VPK, {}, this);
	});
	debugDialogsMenu->addAction("PackFile Options Dialog [ZIP/BSP]", [this] {
		(void) PackFileOptionsDialog::getPackFileOptions(PackFileType::ZIP, {}, this);
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
	this->searchBar->setPlaceholderText(tr("Search..."));
	QObject::connect(this->searchBar, &QLineEdit::editingFinished, this, [this] {
		this->entryTree->setSearchQuery(this->searchBar->text());
		this->fileViewer->setSearchQuery(this->searchBar->text());
	});
	leftPaneLayout->addWidget(this->searchBar);

	this->entryTree = new EntryTree(this, leftPane);
	this->entryTree->setAutoExpandDirectoryOnClick(Options::get<bool>(OPT_ENTRY_TREE_AUTO_EXPAND));
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
	if (Options::get<bool>(OPT_ENTRY_TREE_AUTO_COLLAPSE)) {
		splitter->setSizes({0, splitter->size().width()});
	}

	this->statusText = new QLabel(this->statusBar());
	this->statusProgressBar = new QProgressBar(this->statusBar());

	this->statusBar()->addPermanentWidget(this->statusText, 1);
	this->statusBar()->addPermanentWidget(this->statusProgressBar, 1);

	(void) this->clearContents();

	// Update the theme for anything relying on that
	emit this->themeUpdated();

	// Load the pack file if given one through the command-line or double-clicking a file
	// An error here means shut the application down
	const auto& args = QApplication::arguments();
	if ((args.length() > 1 && QFile::exists(args[1])) && !this->loadPackFile(args[1])) {
		exit(1);
	}

	if (!Options::get<bool>(OPT_DISABLE_STARTUP_UPDATE_CHECK)) {
		this->checkForNewUpdate(true);
	}
}

void Window::newVPK(bool fromDirectory, const QString& startPath) {
	if (this->modified && this->promptUserToKeepModifications()) {
		return;
	}

	auto vpkOptions = NewVPKOptionsDialog::getNewVPKOptions(fromDirectory, {}, false, this);
	if (!vpkOptions) {
		return;
	}
	auto [options, singleFile] = *vpkOptions;

	auto dirPath = fromDirectory ? QFileDialog::getExistingDirectory(this, tr("Use This Folder"), startPath) : "";
	if (fromDirectory && dirPath.isEmpty()) {
		return;
	}

	QString vpkSaveFilePath = std::filesystem::path{dirPath.toLocal8Bit().constData()}.parent_path().string().c_str();
	vpkSaveFilePath += QDir::separator();
	vpkSaveFilePath += (std::filesystem::path{dirPath.toLocal8Bit().constData()}.stem().string() + (singleFile || dirPath.endsWith("_dir") ? ".vpk" : "_dir.vpk")).c_str();
	auto vpkPath = QFileDialog::getSaveFileName(this, tr("Save New VPK"), fromDirectory ? vpkSaveFilePath : startPath, "Valve Pack File (*.vpk)");
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
		this->createVPKFromDirWorkerThread = new QThread(this);
		auto* worker = new IndeterminateProgressWorker();
		worker->moveToThread(this->createVPKFromDirWorkerThread);
		// Cringe compiler moment in the lambda capture list
		QObject::connect(this->createVPKFromDirWorkerThread, &QThread::started, worker, [worker, vpkPath, dirPath, singleFile_=singleFile, options_=options] {
			worker->run([vpkPath, dirPath, singleFile_, options_] {
				(void) VPK::createFromDirectory(vpkPath.toLocal8Bit().constData(), dirPath.toLocal8Bit().constData(), singleFile_, options_);
			});
		});
		QObject::connect(worker, &IndeterminateProgressWorker::taskFinished, this, [this, vpkPath] {
			// Kill thread
			this->createVPKFromDirWorkerThread->quit();
			this->createVPKFromDirWorkerThread->wait();
			delete this->createVPKFromDirWorkerThread;
			this->createVPKFromDirWorkerThread = nullptr;

			// loadVPK freezes them right away again
			// this->freezeActions(false);
			this->loadPackFile(vpkPath);
		});
		this->createVPKFromDirWorkerThread->start();
	} else {
		(void) VPK::createEmpty(vpkPath.toLocal8Bit().constData(), options);
		this->loadPackFile(vpkPath);
	}
}

void Window::openPackFile(const QString& startPath, const QString& filePath) {
	auto path = filePath;
	if (path.isEmpty()) {
		auto supportedExtensions = PackFile::getSupportedFileTypes();
		QString filter = "Supported Files (";
		for (int i = 0; i < supportedExtensions.size(); i++) {
			if (i != 0) {
				filter += ' ';
			}
			filter += ('*' + supportedExtensions[i]).c_str();
		}
		filter += ")";
		path = QFileDialog::getOpenFileName(this, tr("Open Pack File"), startPath, filter);
	}
	if (path.isEmpty()) {
		return;
	}
	this->loadPackFile(path);
}

void Window::savePackFile(bool saveAs, bool async) {
	QString savePath = "";
	if (saveAs) {
		savePath = QFileDialog::getExistingDirectory(this, tr("Save to..."));
		if (savePath.isEmpty()) {
			return;
		}
	}

	// Set up progress bar
	this->statusText->hide();
	this->statusProgressBar->show();

	// Get progress bar maximum
	int progressBarMax = static_cast<int>(this->packFile->getEntryCount());

	// Show progress indicator
	this->statusProgressBar->setRange(0, progressBarMax);
	this->statusProgressBar->setValue(0);

	this->freezeActions(true);

	// Set up thread
	auto* worker = new SavePackFileWorker();
	if (async) {
		this->savePackFileWorkerThread = new QThread(this);
		worker->moveToThread(this->savePackFileWorkerThread);
		QObject::connect(this->savePackFileWorkerThread, &QThread::started, worker, [this, worker, savePath] {
			worker->run(this, savePath);
		});
	}
	QObject::connect(worker, &SavePackFileWorker::progressUpdated, this, [this, progressBarMax](int value) {
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
	QObject::connect(worker, &SavePackFileWorker::taskFinished, this, [this, async](bool success) {
		// Kill thread
		if (async) {
			this->savePackFileWorkerThread->quit();
			this->savePackFileWorkerThread->wait();
			delete this->savePackFileWorkerThread;
			this->savePackFileWorkerThread = nullptr;
		}

		this->freezeActions(false);

		this->resetStatusBar();

		if (!success) {
			QMessageBox::warning(this, tr("Could not save!"),
			                     tr("An error occurred while saving changes to the file. Check that you have permission to write to it, and that no other application is using it."));
		} else {
			this->markModified(false);
		}
	});

	if (async) {
		this->savePackFileWorkerThread->start();
	} else {
		worker->run(this, savePath, false);
	}
}

void Window::saveAsPackFile(bool async) {
	this->savePackFile(true, async);
}

void Window::closePackFile() {
	if (this->clearContents()) {
		this->packFile = nullptr;
	}
}

void Window::checkForNewUpdate(bool hidden) const {
	QNetworkRequest request{QUrl(QString(PROJECT_HOMEPAGE_API.data()) + "/releases/latest")};
	request.setAttribute(QNetworkRequest::Attribute::User, QVariant::fromValue(hidden));
	this->checkForNewUpdateNetworkManager->get(request);
}

void Window::checkForUpdatesReply(QNetworkReply* reply) {
	const auto hidden = reply->request().attribute(QNetworkRequest::Attribute::User).toBool();

	if (reply->error() != QNetworkReply::NoError) {
		if (!hidden) {
			QMessageBox::critical(this, tr("Error"), tr("Error occurred checking for updates!"));
		}
		return;
	}
	const auto parseFailure = [this, hidden] {
		if (!hidden) {
			QMessageBox::critical(this, tr("Error"), tr("Invalid JSON response was retrieved checking for updates!"));
		}
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

	if (!release.contains("body") || !release["body"].isString()) {
		return parseFailure();
	}
	auto details = release["body"].toString();

	if (versionTag == QString("v") + PROJECT_VERSION.data()) {
		if (!hidden) {
			QMessageBox::information(this, tr("No New Updates"), tr("You are using the latest version of the software."));
		}
		return;
	}
	NewUpdateDialog::getNewUpdatePrompt(url, versionName, details, this);
}

bool Window::isReadOnly() const {
	return !this->packFile || this->packFile->isReadOnly();
}

void Window::setProperties() {
	auto options = PackFileOptionsDialog::getPackFileOptions(this->packFile->getType(), this->packFile->getOptions(), this);
	if (!options) {
		return;
	}

	if (auto type = this->packFile->getType(); type == PackFileType::VPK) {
		auto& vpk = dynamic_cast<VPK&>(*this->packFile);
		vpk.setVersion(options->vpk_version);
	}
#ifdef VPKEDIT_ZIP_COMPRESSION
	else if (type == PackFileType::BSP || type == PackFileType::ZIP) {
		auto& zip = dynamic_cast<ZIP&>(*this->packFile);
		zip.setCompressionMethod(options->zip_compressionMethod);
	}
#endif

	this->resetStatusBar();

	this->markModified(true);
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
	prefilledPath += std::filesystem::path{filepath.toLocal8Bit().constData()}.filename().string().c_str();

	QString entryPath = prefilledPath;
	EntryOptions options;

	if (showOptions || Options::get<bool>(OPT_ADVANCED_FILE_PROPS)) {
		auto newEntryOptions = EntryOptionsDialog::getEntryOptions(false, false, prefilledPath, this->packFile->getType(), {}, this);
		if (!newEntryOptions) {
			return;
		}
		entryPath = std::get<0>(*newEntryOptions);
		options = std::get<1>(*newEntryOptions);
	}

	if (!this->packFile->isCaseSensitive()) {
		entryPath = entryPath.toLower();
	}

	this->packFile->removeEntry(entryPath.toLocal8Bit().constData());
	this->packFile->addEntry(entryPath.toLocal8Bit().constData(), filepath.toLocal8Bit().constData(), options);
	this->entryTree->addEntry(entryPath);
	this->fileViewer->addEntry(*this->packFile, entryPath);
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
	prefilledPath += std::filesystem::path{dirpath.toLocal8Bit().constData()}.filename().string().c_str();

	QString parentEntryPath = prefilledPath;
	EntryOptions options;

	if (showOptions || Options::get<bool>(OPT_ADVANCED_FILE_PROPS)) {
		auto newEntryOptions = EntryOptionsDialog::getEntryOptions(false, true, prefilledPath, this->packFile->getType(), {}, this);
		if (!newEntryOptions) {
			return;
		}
		parentEntryPath = std::get<0>(*newEntryOptions);
		options = std::get<1>(*newEntryOptions);
	}

	QDirIterator it(dirpath, QDir::Files | QDir::Readable, QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
	while (it.hasNext()) {
		QString subEntryPathFS = it.next();
		QString subEntryPath = parentEntryPath + subEntryPathFS.sliced(dirpath.length());

		if (!this->packFile->isCaseSensitive()) {
			subEntryPath = subEntryPath.toLower();
		}

		this->packFile->removeEntry(subEntryPath.toLocal8Bit().constData());
		this->packFile->addEntry(subEntryPath.toLocal8Bit().constData(), subEntryPathFS.toLocal8Bit().constData(), options);
		this->entryTree->addEntry(subEntryPath);
		this->fileViewer->addEntry(*this->packFile, subEntryPath);
	}
	this->markModified(true);
}

bool Window::removeFile(const QString& path) {
	if (!this->packFile->removeEntry(path.toLocal8Bit().constData())) {
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
	auto entry = this->packFile->findEntry(oldPath.toLocal8Bit().constData());
	if (!entry) {
		QMessageBox::critical(this, tr("Error"), tr("Unable to edit file at \"%1\": could not find file!").arg(oldPath));
		return;
	}
	auto data = this->packFile->readEntry(*entry);
	if (!data) {
		QMessageBox::critical(this, tr("Error"), tr("Unable to edit file at \"%1\": could not read file data!").arg(oldPath));
		return;
	}

	// Get new properties
	const auto options = EntryOptionsDialog::getEntryOptions(true, false, oldPath, this->packFile->getType(), {}, this);
	if (!options) {
		return;
	}
	const auto [newPath, entryOptions] = *options;

	// Remove file
	this->requestEntryRemoval(oldPath);

	// Add new file with the same info and data at the new path
	this->packFile->addEntry(newPath.toLocal8Bit().constData(), std::move(data.value()), entryOptions);
	this->entryTree->addEntry(newPath);
	this->fileViewer->addEntry(*this->packFile, newPath);
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

	QStringList paths;
	for (const auto& [directory, entries] : this->packFile->getBakedEntries()) {
		if (QString(directory.c_str()).startsWith(oldPath)) {
			for (const auto& entry : entries) {
				paths.push_back(QString(directory.c_str()) + '/' + entry.getFilename().c_str());
			}
		}
	}
	for (const auto& [directory, entries] : this->packFile->getUnbakedEntries()) {
		if (QString(directory.c_str()).startsWith(oldPath)) {
			for (const auto& entry : entries) {
				paths.push_back(QString(directory.c_str()) + '/' + entry.getFilename().c_str());
			}
		}
	}

	for (const auto& path : paths) {
		// Get data
		auto entry = this->packFile->findEntry(path.toLocal8Bit().constData());
		if (!entry) {
			continue;
		}
		auto entryData = this->packFile->readEntry(*entry);
		if (!entryData) {
			continue;
		}

		// Remove file
		this->requestEntryRemoval(path);

		// Calculate new path
		QString newEntryPath = newPath + path.sliced(oldPath.length());

		// Add new file with the same info and data at the new path
		this->packFile->addEntry(newEntryPath.toLocal8Bit().constData(), std::move(entryData.value()), {
			.vpk_saveToDirectory = entry->vpk_archiveIndex == VPK_DIR_INDEX,
			.vpk_preloadBytes = static_cast<unsigned int>(entry->vpk_preloadedData.size()),
		});
		this->entryTree->addEntry(newEntryPath);
		this->fileViewer->addEntry(*this->packFile, newEntryPath);
	}
	this->markModified(true);
}

void Window::about() {
	QString creditsText = QString("# %1\n").arg(PROJECT_TITLE.data()) + '*' +
			tr("Created by %1").arg("[craftablescience](https://github.com/craftablescience)") +
			"*\n<br/>\n";
	QFile creditsFile(":/CREDITS.md");
	if (creditsFile.open(QIODevice::ReadOnly)) {
		QTextStream in(&creditsFile);
		creditsText += in.readAll();
		creditsFile.close();
	}

	QMessageBox about(this);
	about.setWindowTitle(tr("About"));
	about.setIconPixmap(QIcon(":/logo.png").pixmap(64, 64));
	about.setTextFormat(Qt::TextFormat::MarkdownText);
	about.setText(creditsText);
	about.exec();
}

void Window::generateKeyPairFiles(const QString& name) {
	auto path = name;
	if (path.isEmpty()) {
		path = QInputDialog::getText(this, tr("Keypair Filename"), tr("Name of the keypair files to generate:"));
		if (path.isEmpty()) {
			return;
		}
		path = (std::filesystem::path{this->packFile->getFilepath()}.parent_path() / path.toLocal8Bit().constData()).string().c_str();
	}
	VPK::generateKeyPairFiles(path.toLocal8Bit().constData());
}

void Window::signPackFile(const QString& privateKeyLocation) {
	auto privateKeyPath = privateKeyLocation;
	if (privateKeyPath.isEmpty()) {
		privateKeyPath = QFileDialog::getOpenFileName(this, tr("Open Private Key File"),
													  std::filesystem::path{this->packFile->getFilepath()}.parent_path().string().c_str(),
													  "Private Key (*.privatekey.vdf);;All Files (*)");
	}
	if (privateKeyPath.isEmpty()) {
		return;
	}
	if (this->packFile->getType() == PackFileType::VPK && dynamic_cast<VPK&>(*this->packFile).sign(privateKeyPath.toLocal8Bit().constData())) {
		QMessageBox::information(this, tr("Success"), tr("Successfully signed the pack file."));
	} else {
		QMessageBox::information(this, tr("Error"), tr("Failed to sign the pack file! Check the file contains both the private key and public key."));
	}
}

std::optional<std::vector<std::byte>> Window::readBinaryEntry(const QString& path) const {
	auto entry = this->packFile->findEntry(path.toLocal8Bit().constData());
	if (!entry) {
		return std::nullopt;
	}
	return this->packFile->readEntry(*entry);
}

std::optional<QString> Window::readTextEntry(const QString& path) const {
	auto entry = this->packFile->findEntry(path.toLocal8Bit().constData());
	if (!entry) {
		return std::nullopt;
	}
	auto binData = this->packFile->readEntry(*entry);
	if (!binData) {
		return std::nullopt;
	}
	QByteArrayView textBuffer{reinterpret_cast<const char*>(binData->data()), static_cast<qsizetype>(binData->size())};
	auto potentialEncoding = QStringConverter::encodingForData(textBuffer);
	if (!potentialEncoding) {
		return {textBuffer.toByteArray()};
	}
	QStringDecoder utf8Converter{*potentialEncoding};
	return utf8Converter(textBuffer);
}

void Window::selectEntryInEntryTree(const QString& path) const {
	this->entryTree->selectEntry(path);
}

void Window::selectEntryInFileViewer(const QString& path) const {
	this->fileViewer->displayEntry(path, *this->packFile);
}

void Window::selectDirInFileViewer(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths) const {
	this->fileViewer->displayDir(path, subfolders, entryPaths, *this->packFile);
}

bool Window::hasEntry(const QString& path) const {
	return this->entryTree->hasEntry(path);
}

void Window::selectSubItemInDir(const QString& path) const {
	this->entryTree->selectSubItem(path);
}

void Window::extractVirtualFile(const QString& name, QString savePath) {
	std::optional<std::vector<std::byte>> data = std::nullopt;
	for (const auto& virtualEntry : this->packFile->getVirtualEntries()) {
		if (virtualEntry.name == name.toLocal8Bit().constData()) {
			data = this->packFile->readVirtualEntry(virtualEntry);
			break;
		}
	}
    if (!data) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to read data for virtual file named \"%1\".").arg(name));
        return;
    }

    if (savePath.isEmpty()) {
        QString filter;
        if (auto index = name.lastIndexOf('.'); index >= 0) {
            auto fileExt = name.sliced(index); // ".ext"
            auto fileExtPretty = fileExt.toUpper();
            fileExtPretty.remove('.');

            filter = fileExtPretty + " (*" + fileExt + ");;All files (*.*)";
        }
        savePath = QFileDialog::getSaveFileName(this, tr("Extract as..."), name, filter);
    }
    if (savePath.isEmpty()) {
        return;
    }

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to write to file at \"%1\".").arg(savePath));
        return;
    }
    auto bytesWritten = file.write(reinterpret_cast<const char*>(data->data()), static_cast<std::streamsize>(data->size()));
    if (bytesWritten != data->size()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to write to file at \"%1\".").arg(savePath));
    }
    file.close();
}

void Window::extractFile(const QString& path, QString savePath) {
	auto entry = this->packFile->findEntry(path.toLocal8Bit().constData());
	if (!entry) {
		QMessageBox::critical(this, tr("Error"), tr("Failed to find file at \"%1\".").arg(path));
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

void Window::extractFilesIf(const std::function<bool(const QString&)>& predicate, const QString& savePath) {
	QString saveDir = savePath;
	if (saveDir.isEmpty()) {
		saveDir = QFileDialog::getExistingDirectory(this, tr("Extract to..."));
	}
	if (saveDir.isEmpty()) {
		return;
	}

	// Set up progress bar
	this->statusText->hide();
	this->statusProgressBar->show();

	// Get progress bar maximum
	int progressBarMax = 0;
	for (const auto& [directory, entries] : this->packFile->getBakedEntries()) {
		if (predicate(QString(directory.c_str()))) {
			progressBarMax += static_cast<int>(entries.size());
		}
	}
	for (const auto& [directory, entries] : this->packFile->getUnbakedEntries()) {
		if (predicate(QString(directory.c_str()))) {
			progressBarMax += static_cast<int>(entries.size());
		}
	}

	this->statusProgressBar->setRange(0, progressBarMax);
	this->statusProgressBar->setValue(0);

	this->freezeActions(true);

	// Set up thread
	this->extractPackFileWorkerThread = new QThread(this);
	auto* worker = new ExtractPackFileWorker();
	worker->moveToThread(this->extractPackFileWorkerThread);
	QObject::connect(this->extractPackFileWorkerThread, &QThread::started, worker, [this, worker, saveDir, predicate] {
		worker->run(this, saveDir, predicate);
	});
	QObject::connect(worker, &ExtractPackFileWorker::progressUpdated, this, [this](int value) {
		this->statusProgressBar->setValue(value);
	});
	QObject::connect(worker, &ExtractPackFileWorker::taskFinished, this, [this] {
		// Kill thread
		this->extractPackFileWorkerThread->quit();
		this->extractPackFileWorkerThread->wait();
		delete this->extractPackFileWorkerThread;
		this->extractPackFileWorkerThread = nullptr;

		this->freezeActions(false);

		this->resetStatusBar();
	});
	this->extractPackFileWorkerThread->start();
}

void Window::extractDir(const QString& path, const QString& saveDir) {
	this->extractFilesIf([path](const QString& dir) { return dir.startsWith(path); }, saveDir);
}

void Window::extractPaths(const QStringList& paths, const QString& saveDir) {
	this->entryTree->extractEntries(paths, saveDir);
}

void Window::createDrag(const QStringList& paths) {
	this->entryTree->createDrag(paths);
}

void Window::extractAll(QString saveDir) {
	if (saveDir.isEmpty()) {
		saveDir = QFileDialog::getExistingDirectory(this, tr("Extract to..."));
	}
	if (saveDir.isEmpty()) {
		return;
	}
	saveDir += '/';
	saveDir += this->packFile->getFilestem().c_str();

	this->extractFilesIf([](const QString&) { return true; }, saveDir);
}

void Window::setDropEnabled(bool dropEnabled_) {
	this->dropEnabled = dropEnabled_;
}

void Window::markModified(bool modified_) {
	if (this->isReadOnly()) {
		return;
	}

	this->modified = modified_;

	if (this->modified) {
		this->setWindowTitle(PROJECT_TITLE.data() + QString(" (*)"));
	} else {
		this->setWindowTitle(PROJECT_TITLE.data());
	}

	this->saveAction->setDisabled(!this->modified);
}

bool Window::promptUserToKeepModifications() {
	auto response = QMessageBox::warning(this,
            tr("Save changes?"),
            tr("This file has unsaved changes! Would you like to save these changes first?"),
            QMessageBox::Ok | QMessageBox::Discard | QMessageBox::Cancel);
	switch (response) {
		case QMessageBox::Cancel:
			return true;
		case QMessageBox::Discard:
			return false;
		case QMessageBox::Ok:
			this->savePackFile(false, false);
			return false;
		default:
			break;
	}
	return true;
}

bool Window::clearContents() {
	if (this->modified && this->promptUserToKeepModifications()) {
		return false;
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

	return true;
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
	if (!this->dropEnabled || !event->mimeData()->hasUrls() || !this->fileViewer->isDirPreviewVisible()) {
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
	this->openAction->setDisabled(freeze && freezeCreationActions);
	this->openRelativeToMenu->setDisabled(freeze && freezeCreationActions);
	this->openRecentMenu->setDisabled(freeze && freezeCreationActions);
	this->saveAction->setDisabled(freeze || !this->modified);
	this->saveAsAction->setDisabled(freeze);
	this->closeFileAction->setDisabled(freeze);
	this->extractAllAction->setDisabled(freeze);
	this->addFileAction->setDisabled(freeze);
	this->addDirAction->setDisabled(freeze);
	this->setPropertiesAction->setDisabled(freeze);
	this->toolsGeneralMenu->setDisabled(freeze);
	this->toolsVPKMenu->setDisabled(freeze || (!this->packFile || this->packFile->getType() != PackFileType::VPK));

	this->searchBar->setDisabled(freeze);
	this->entryTree->setDisabled(freeze);
	this->fileViewer->setDisabled(freeze);
}

void Window::freezeModifyActions(bool readOnly) const {
	if (readOnly) {
		this->saveAction->setDisabled(readOnly);
		this->saveAsAction->setDisabled(readOnly);
		this->addFileAction->setDisabled(readOnly);
		this->addDirAction->setDisabled(readOnly);
		this->setPropertiesAction->setDisabled(readOnly);
	}
}

bool Window::loadPackFile(const QString& path) {
	if (!this->clearContents()) {
		return false;
	}
	this->freezeActions(true);

	auto recentPaths = Options::get<QStringList>(STR_OPEN_RECENT);

	QString fixedPath = QDir(path).absolutePath();
	fixedPath.replace('\\', '/');

	this->packFile = PackFile::open(fixedPath.toLocal8Bit().constData());
	if (!this->packFile) {
		// Remove from recent paths if it's there
		if (recentPaths.contains(fixedPath)) {
			recentPaths.removeAt(recentPaths.indexOf(fixedPath));
			Options::set(STR_OPEN_RECENT, recentPaths);
			this->rebuildOpenRecentMenu(recentPaths);
		}

		QMessageBox::critical(this, tr("Error"), tr("Unable to load this file. Please ensure that a game or another application is not using the file."));
		(void) this->clearContents();
		return false;
	}

	// Add to recent paths
	QString loadedPath{this->packFile->getFilepath().data()};
	if (!recentPaths.contains(loadedPath)) {
		recentPaths.push_front(loadedPath);
		if (recentPaths.size() > 10) {
			recentPaths.pop_back();
		}
		Options::set(STR_OPEN_RECENT, recentPaths);
		this->rebuildOpenRecentMenu(recentPaths);
	} else if (auto pathIndex = recentPaths.indexOf(loadedPath); pathIndex > 0) {
		recentPaths.remove(pathIndex);
		recentPaths.push_front(loadedPath);
		Options::set(STR_OPEN_RECENT, recentPaths);
		this->rebuildOpenRecentMenu(recentPaths);
	}

	this->statusText->hide();
	this->statusProgressBar->show();
	this->statusBar()->show();

	this->entryTree->loadPackFile(*this->packFile, this->statusProgressBar, [this, path] {
		this->freezeActions(false);
		this->freezeModifyActions(this->isReadOnly());

		this->resetStatusBar();
	});

	return true;
}

void Window::rebuildOpenInMenu() {
	this->openRelativeToMenu->clear();
	auto* loadingGamesAction = this->openRelativeToMenu->addAction(tr("Loading installed games..."));
	loadingGamesAction->setDisabled(true);

	// Set up thread
	this->scanSteamGamesWorkerThread = new QThread(this);
	auto* worker = new ScanSteamGamesWorker();
	worker->moveToThread(this->scanSteamGamesWorkerThread);
	QObject::connect(this->scanSteamGamesWorkerThread, &QThread::started, worker, [worker] {
		worker->run();
	});
	QObject::connect(worker, &ScanSteamGamesWorker::taskFinished, this, [this](const QList<std::tuple<QString, QIcon, QDir>>& sourceGames) {
		// Add them to the menu
		this->openRelativeToMenu->clear();
		if (!sourceGames.empty()) {
			for (const auto& [gameName, icon, relativeDirectoryPath] : sourceGames) {
				const auto relativeDirectory = relativeDirectoryPath.path();
				this->openRelativeToMenu->addAction(icon, gameName, [this, relativeDirectory] {
					this->openPackFile(relativeDirectory);
				});
			}
		} else {
			auto* noGamesDetectedAction = this->openRelativeToMenu->addAction(tr("No games detected."));
			noGamesDetectedAction->setDisabled(true);
		}

		// Kill thread
		this->scanSteamGamesWorkerThread->quit();
		this->scanSteamGamesWorkerThread->wait();
		delete this->scanSteamGamesWorkerThread;
		this->scanSteamGamesWorkerThread = nullptr;
	});
	this->scanSteamGamesWorkerThread->start();
}

void Window::rebuildOpenRecentMenu(const QStringList& paths) {
	this->openRecentMenu->clear();
	if (paths.empty()) {
		auto* openRecentVPKMenuNoRecentFilesAction = this->openRecentMenu->addAction(tr("No recent files."));
		openRecentVPKMenuNoRecentFilesAction->setDisabled(true);
		return;
	}
	for (int i = 0; i < paths.size(); i++) {
		this->openRecentMenu->addAction(("&%1: \"" + paths[i] + "\"").arg((i + 1) % 10), [this, path=paths[i]] {
			this->loadPackFile(path);
		});
	}
	this->openRecentMenu->addSeparator();
	this->openRecentMenu->addAction(tr("Clear"), [this] {
		Options::set(STR_OPEN_RECENT, QStringList{});
		this->rebuildOpenRecentMenu({});
	});
}

void Window::writeEntryToFile(const QString& path, const Entry& entry) {
	auto data = this->packFile->readEntry(entry);
	if (!data) {
		QMessageBox::critical(this, tr("Error"), tr("Failed to read data for \"%1\". Please ensure that a game or another application is not using the file.").arg(entry.path.c_str()));
		return;
	}
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		QMessageBox::critical(this, tr("Error"), tr("Failed to write to file at \"%1\".").arg(path));
		return;
	}
	auto bytesWritten = file.write(reinterpret_cast<const char*>(data->data()), static_cast<std::streamsize>(entry.length));
	if (bytesWritten != entry.length) {
		QMessageBox::critical(this, tr("Error"), tr("Failed to write to file at \"%1\".").arg(path));
	}
	file.close();
}

void Window::resetStatusBar() {
	QString packFileStatus(std::string{*this->packFile}.c_str());
	packFileStatus
		.replace("AppID", tr("AppID"))
		.replace("App Version", tr("App Version"))
		.replace("Godot Version", tr("Godot Version"))
		.replace("Version", tr("Version"))
		.replace("Map Revision", tr("Map Revision"))
		.replace("Addon Name:", tr("Addon Name:"))
		.replace("Embedded", tr("Embedded"))
		.replace("Encrypted", tr("Encrypted"));
	this->statusText->setText(' ' + tr("Loaded") + ' ' + packFileStatus);
	this->statusText->show();
	this->statusProgressBar->hide();
}

void IndeterminateProgressWorker::run(const std::function<void()>& fn) {
	fn();
	emit this->taskFinished();
}

void SavePackFileWorker::run(Window* window, const QString& savePath, bool async) {
	std::unique_ptr<QEventLoop> loop;
	if (!async) {
		loop = std::make_unique<QEventLoop>();
	}
	int currentEntry = 0;
	bool success = window->packFile->bake(savePath.toLocal8Bit().constData(), [this, loop_=loop.get(), &currentEntry](const std::string&, const Entry&) {
		emit this->progressUpdated(++currentEntry);
		if (loop_) {
			loop_->processEvents();
		}
	});
	emit this->taskFinished(success);
}

void ExtractPackFileWorker::run(Window* window, const QString& saveDir, const std::function<bool(const QString&)>& predicate) {
	int currentEntry = 0;
	for (const auto& [directory, entries] : window->packFile->getBakedEntries()) {
		QString dir(directory.c_str());
		if (!predicate(dir)) {
			continue;
		}

#ifdef _WIN32
		// Remove bad characters from the filepath
		dir.replace('<', "_LT_");
		dir.replace('>', "_GT_");
		dir.replace(':', "_COLON_");
		dir.replace('"', "_QUOT_");
		dir.replace('|', "_BAR_");
		dir.replace('?', "_QMARK_");
		dir.replace('*', "_AST_");
#endif

		QDir qDir;
		if (!qDir.mkpath(saveDir + QDir::separator() + dir)) {
			QMessageBox::critical(window, tr("Error"), tr("Failed to create directory."));
			return;
		}

		for (const auto& entry : entries) {
			std::string filename{entry.getFilename()};
#ifdef _WIN32
			{
				std::filesystem::path path{filename};
				auto extension = path.extension().string();
				QString stem = path.stem().string().c_str();
				stem = stem.toUpper();

				// Replace bad filenames
				if (stem == "CON") {
					filename = "_CON_" + extension;
				} else if (stem == "PRN") {
					filename = "_PRN_" + extension;
				} else if (stem == "AUX") {
					filename = "_AUX_" + extension;
				} else if (stem == "NUL") {
					filename = "_NUL_" + extension;
				} else if (stem.startsWith("COM") && stem.length() == 4 && stem[3].isDigit() && stem[3] != '0') {
					filename = "_COM";
					filename += stem[3].toLatin1();
					filename += '_';
					filename += extension;
				} else if (stem.startsWith("LPT") && stem.length() == 4 && stem[3].isDigit() && stem[3] != '0') {
					filename = "_LPT";
					filename += stem[3].toLatin1();
					filename += '_';
					filename += extension;
				}

				// Files cannot end with a period - weird
				if (extension == ".") {
					filename.pop_back();
				}
			}
#endif
			auto filePath = saveDir + QDir::separator() + dir + QDir::separator() + filename.c_str();
			window->writeEntryToFile(filePath, entry);
			emit this->progressUpdated(++currentEntry);
		}
	}
	for (const auto& [directory, entries] : window->packFile->getUnbakedEntries()) {
		QString dir(directory.c_str());
		if (!predicate(dir)) {
			continue;
		}

		QDir qDir;
		if (!qDir.mkpath(saveDir + QDir::separator() + dir)) {
			QMessageBox::critical(window, tr("Error"), tr("Failed to create directory."));
			return;
		}

		for (const auto& entry : entries) {
			auto filePath = saveDir + QDir::separator() + dir + QDir::separator() + entry.getFilename().c_str();
			window->writeEntryToFile(filePath, entry);
			emit this->progressUpdated(++currentEntry);
		}
	}
	emit this->taskFinished();
}

void ScanSteamGamesWorker::run() {
	QList<std::tuple<QString, QIcon, QDir>> sourceGames;

	if (Options::get<bool>(OPT_DISABLE_STEAM_SCANNER)) {
		emit this->taskFinished(sourceGames);
		return;
	}

	SAPP sapp;
	if (!sapp) {
		emit this->taskFinished(sourceGames);
		return;
	}

	// Add Steam games
	for (auto appID : sapp.getInstalledApps()) {
		if (!sapp.isAppUsingSourceEngine(appID) && !sapp.isAppUsingSource2Engine(appID)) {
			continue;
		}
		auto iconPath = sapp.getAppIconPath(appID);
		sourceGames.emplace_back(sapp.getAppName(appID).data(), iconPath.empty() ? QIcon(":/icons/missing_app.png") : QIcon(iconPath.c_str()), sapp.getAppInstallDir(appID).c_str());
	}

	// Add mods in the sourcemods directory
	for (const auto& modDir : std::filesystem::directory_iterator{sapp.getSteamSourceModDir()}) {
		if (!modDir.is_directory()) {
			continue;
		}

		const auto gameInfoPath = (modDir.path() / "gameinfo.txt").string();
		if (!std::filesystem::exists(gameInfoPath)) {
			continue;
		}

		std::ifstream gameInfoFile{gameInfoPath};
		auto gameInfoSize = std::filesystem::file_size(gameInfoPath);
		std::string gameInfoData;
		gameInfoData.resize(gameInfoSize);
		gameInfoFile.read(gameInfoData.data(), static_cast<std::streamsize>(gameInfoSize));

		KeyValueRoot gameInfoRoot{gameInfoData.c_str()};
		if (!gameInfoRoot.IsValid()) {
			continue;
		}
		auto& gameInfo = gameInfoRoot.Get("GameInfo");
		if (!gameInfo.IsValid()) {
			continue;
		}
		auto& gameInfoName = gameInfo.Get("game");
		auto& gameInfoIconPath = gameInfo.Get("icon");

		std::string modName;
		if (gameInfoName.IsValid()) {
			modName = gameInfoName.Value().string;
		} else {
			modName = std::filesystem::path{gameInfoPath}.parent_path().filename().string();
		}

		std::string modIconPath;
		if (gameInfoIconPath.IsValid()) {
			if (auto modIconBigPath = (modDir.path() / (std::string{gameInfoIconPath.Value().string} + "_big.tga")); std::filesystem::exists(modIconBigPath)) {
				modIconPath = modIconBigPath.string();
			} else if (auto modIconRegularPath = (modDir.path() / (std::string{gameInfoIconPath.Value().string} + ".tga")); std::filesystem::exists(modIconRegularPath)) {
				modIconPath = modIconRegularPath.string();
			}
		}

		std::optional<QImage> modIconTGA = modIconPath.empty() ? std::nullopt : TGADecoder::decodeImage(modIconPath.c_str());
		sourceGames.emplace_back(modName.c_str(), modIconTGA ? QIcon(QPixmap::fromImage(*modIconTGA)) : QIcon(":/icons/missing_app.png"), modDir.path().string().c_str());
	}

	// Replace & with && in game names
	for (auto& games : sourceGames) {
		// Having an & before a character makes that the shortcut character and hides the &, so we need to escape it
		std::get<0>(games).replace("&", "&&");
	}

	// Sort games and return
	std::sort(sourceGames.begin(), sourceGames.end(), [](const auto& lhs, const auto& rhs) {
		return std::get<0>(lhs) < std::get<0>(rhs);
	});
	emit this->taskFinished(sourceGames);
}
