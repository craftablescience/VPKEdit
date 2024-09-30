#include "Window.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>

#include <kvpp/kvpp.h>
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
#include <QProgressDialog>
#include <QSplitter>
#include <QStatusBar>
#include <QStringDecoder>
#include <QStyleFactory>
#include <QThread>
#include <QTimer>
#include <steampp/steampp.h>
#include <vpkpp/vpkpp.h>

#include <Version.h>

#include "dialogs/ControlsDialog.h"
#include "dialogs/EntryOptionsDialog.h"
#include "dialogs/NewUpdateDialog.h"
#include "dialogs/VerifyChecksumsDialog.h"
#include "dialogs/VerifySignatureDialog.h"
#include "dialogs/VICEDialog.h"
#include "extensions/Folder.h"
#include "utility/DiscordPresence.h"
#include "utility/Options.h"
#include "utility/TGADecoder.h"
#include "EntryTree.h"
#include "FileViewer.h"

using namespace kvpp;
using namespace steampp;
using namespace vpkpp;

Window::Window(QWidget* parent)
		: QMainWindow(parent)
		, modified(false)
		, dropEnabled(true) {
	this->setWindowTitle(PROJECT_TITLE.data());
	this->setWindowIcon(QIcon(":/logo.png"));

	const auto showRestartWarning = [this] {
		static bool shownRestartMessage = false;
		if (!shownRestartMessage) {
			QMessageBox::warning(this, tr("Restart Required"), tr("The application must be restarted for these settings to take effect."));
			shownRestartMessage = true;
		}
	};

	// File menu
	auto* fileMenu = this->menuBar()->addMenu(tr("File"));

	this->createEmptyMenu = fileMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileIcon), tr("Create..."));
	this->createEmptyMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "BMZ", [this] {
		this->newBMZ(false);
	});
	this->createEmptyMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "FPX", [this] {
		this->newFPX(false);
	});
	this->createEmptyMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "PAK", [this] {
		this->newPAK(false);
	});
	this->createEmptyMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "PCK", [this] {
		this->newPCK(false);
	});
	this->createEmptyMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "VPK", [this] {
		this->newVPK(false);
	});
	this->createEmptyMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "WAD3", [this] {
		this->newWAD3(false);
	});
	this->createEmptyMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "ZIP", [this] {
		this->newZIP(false);
	});

	this->createFromDirMenu = fileMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileIcon), tr("Create from Folder..."));
	this->createFromDirMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "BMZ", [this] {
		this->newBMZ(true);
	});
	this->createFromDirMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "FPX", [this] {
		this->newFPX(true);
	});
	this->createFromDirMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "PAK", [this] {
		this->newPAK(true);
	});
	this->createFromDirMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "PCK", [this] {
		this->newPCK(true);
	});
	this->createFromDirMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "VPK", [this] {
		this->newVPK(true);
	});
	this->createFromDirMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "WAD3", [this] {
		this->newWAD3(true);
	});
	this->createFromDirMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), "ZIP", [this] {
		this->newZIP(true);
	});

	this->openAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DirIcon), tr("Open..."), Qt::CTRL | Qt::Key_O, [this] {
		this->openPackFile();
	});

	this->openDirAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DirIcon), tr("Open Folder..."), Qt::CTRL | Qt::SHIFT | Qt::Key_O, [this] {
		this->openDir();
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

	this->closeFileAction = fileMenu->addAction(this->style()->standardIcon(QStyle::SP_BrowserReload), tr("Close"), Qt::CTRL | Qt::Key_W, [this] {
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
		(void) PackFileOptionsDialog::getForNew(PackFileType::VPK, false, this);
	});
	debugDialogsMenu->addAction("Create VPK From Folder Options Dialog", [this] {
		(void) PackFileOptionsDialog::getForNew(PackFileType::VPK, true, this);
	});
	debugDialogsMenu->addAction("PackFile Options Dialog [VPK]", [this] {
		(void) PackFileOptionsDialog::getForEdit(PackFileType::VPK, {}, this);
	});
	debugDialogsMenu->addAction("PackFile Options Dialog [ZIP/BSP]", [this] {
		(void) PackFileOptionsDialog::getForEdit(PackFileType::ZIP, {}, this);
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

template<PackFileType Type>
void newPackFile(Window* window, bool fromDirectory, const QString& startPath, const QString& name, const QString& extension) {
	static_assert(Type == PackFileType::FPX || Type == PackFileType::PAK || Type == PackFileType::PCK || Type == PackFileType::VPK || Type == PackFileType::WAD3 || Type == PackFileType::ZIP);

	if (window->isModified() && window->promptUserToKeepModifications()) {
		return;
	}

	auto options = PackFileOptionsDialog::getForNew(Type, fromDirectory, window);
	if (!options) {
		return;
	}

	auto dirPath = fromDirectory ? QFileDialog::getExistingDirectory(window, QObject::tr("Use This Folder"), startPath) : "";
	if (fromDirectory && dirPath.isEmpty()) {
		return;
	}

	QString saveFilePath;
	if (fromDirectory) {
		saveFilePath = std::filesystem::path{dirPath.toLocal8Bit().constData()}.parent_path().string().c_str();
		saveFilePath += QDir::separator();
		if constexpr (Type == PackFileType::FPX || Type == PackFileType::VPK) {
			saveFilePath += std::filesystem::path{dirPath.toLocal8Bit().constData()}.stem().string().c_str() + (((options->vpk_saveSingleFile || dirPath.endsWith("_dir")) ? "" : "_dir") + extension);
		} else {
			saveFilePath += std::filesystem::path{dirPath.toLocal8Bit().constData()}.stem().string().c_str() + extension;
		}
	}
	auto packFilePath = QFileDialog::getSaveFileName(window, QObject::tr("Save New Pack File"), fromDirectory ? saveFilePath : startPath, name + " (*" + extension + ")");
	if (packFilePath.isEmpty()) {
		return;
	}

	std::unique_ptr<PackFile> packFile;
	if constexpr (Type == PackFileType::FPX) {
		packFile = FPX::create(packFilePath.toLocal8Bit().constData());
		if (auto* fpx = dynamic_cast<FPX*>(packFile.get())) {
			fpx->setChunkSize(options->vpk_chunkSize);
		}
	} else if constexpr (Type == PackFileType::PAK) {
		packFile = PAK::create(packFilePath.toLocal8Bit().constData());
	} else if constexpr (Type == PackFileType::PCK) {
		packFile = PCK::create(packFilePath.toLocal8Bit().constData());
	} else if constexpr (Type == PackFileType::VPK) {
		packFile = VPK::create(packFilePath.toLocal8Bit().constData(), options->vpk_version);
		if (auto* vpk = dynamic_cast<VPK*>(packFile.get())) {
			vpk->setChunkSize(options->vpk_chunkSize);
		}
	} else if constexpr (Type == PackFileType::WAD3) {
		packFile = WAD3::create(packFilePath.toLocal8Bit().constData());
	} else if constexpr (Type == PackFileType::ZIP) {
		packFile = ZIP::create(packFilePath.toLocal8Bit().constData());
	} else {
		return;
	}

	if (!fromDirectory) {
		window->loadPackFile(packFilePath, std::move(packFile));
		return;
	}

	// Set up progress bar
	window->statusText->hide();
	window->statusProgressBar->show();
	window->statusBar()->show();

	// Show progress bar is busy
	window->statusProgressBar->setValue(0);
	window->statusProgressBar->setRange(0, 0);

	window->freezeActions(true);

	// Set up thread
	window->createPackFileFromDirWorkerThread = new QThread(window);
	auto* worker = new IndeterminateProgressWorker();
	worker->moveToThread(window->createPackFileFromDirWorkerThread);
	QObject::connect(window->createPackFileFromDirWorkerThread, &QThread::started, worker, [worker, packFilePath, dirPath, options_=*options] {
		worker->run([packFilePath, dirPath, options_] {
			if (auto packFile = PackFile::open(packFilePath.toLocal8Bit().constData())) {
				if (packFile->getType() == PackFileType::FPX) {
					if (auto* fpx = dynamic_cast<FPX*>(packFile.get())) {
						fpx->setChunkSize(options_.vpk_chunkSize);
					}
				} else if (packFile->getType() == PackFileType::VPK) {
					if (auto* vpk = dynamic_cast<VPK*>(packFile.get())) {
						vpk->setChunkSize(options_.vpk_chunkSize);
					}
				}
				packFile->addDirectory("", dirPath.toLocal8Bit().constData(), {
					.vpk_saveToDirectory = options_.vpk_saveSingleFile,
					.vpk_preloadBytes = 0,
				});
				packFile->bake("", {}, nullptr);
			}
		});
	});
	QObject::connect(worker, &IndeterminateProgressWorker::taskFinished, window, [window, packFilePath, options_=*options] {
		// Kill thread
		window->createPackFileFromDirWorkerThread->quit();
		window->createPackFileFromDirWorkerThread->wait();
		delete window->createPackFileFromDirWorkerThread;
		window->createPackFileFromDirWorkerThread = nullptr;

		// loadPackFile freezes them right away again
		// this->freezeActions(false);
		window->loadPackFile(packFilePath);
		if (window->packFile) {
			if (window->packFile->getType() == PackFileType::FPX) {
				if (auto* fpx = dynamic_cast<FPX*>(window->packFile.get())) {
					fpx->setChunkSize(options_.vpk_chunkSize);
				}
			} else if (window->packFile->getType() == PackFileType::VPK) {
				if (auto* vpk = dynamic_cast<VPK*>(window->packFile.get())) {
					vpk->setChunkSize(options_.vpk_chunkSize);
				}
			}
		}
	});
	window->createPackFileFromDirWorkerThread->start();
}

void Window::newBMZ(bool fromDirectory, const QString& startPath) {
	return ::newPackFile<PackFileType::ZIP>(this, fromDirectory, startPath, "BMZ", ".bmz");
}

void Window::newFPX(bool fromDirectory, const QString& startPath) {
	return ::newPackFile<PackFileType::FPX>(this, fromDirectory, startPath, "FPX", ".fpx");
}

void Window::newPAK(bool fromDirectory, const QString& startPath) {
	return ::newPackFile<PackFileType::PAK>(this, fromDirectory, startPath, "PAK", ".pak");
}

void Window::newPCK(bool fromDirectory, const QString& startPath) {
	return ::newPackFile<PackFileType::PCK>(this, fromDirectory, startPath, "PCK", ".pck");
}

void Window::newVPK(bool fromDirectory, const QString& startPath) {
	return ::newPackFile<PackFileType::VPK>(this, fromDirectory, startPath, "VPK", ".vpk");
}

void Window::newWAD3(bool fromDirectory, const QString& startPath) {
	return ::newPackFile<PackFileType::WAD3>(this, fromDirectory, startPath, "WAD3", ".wad");
}

void Window::newZIP(bool fromDirectory, const QString& startPath) {
	return ::newPackFile<PackFileType::ZIP>(this, fromDirectory, startPath, "ZIP", ".zip");
}

void Window::openDir(const QString& startPath, const QString& dirPath) {
	auto path = dirPath;
	if (path.isEmpty()) {
		path = QFileDialog::getExistingDirectory(this, tr("Open Folder"), startPath);
	}
	if (path.isEmpty()) {
		return;
	}
	this->loadDir(path);
}

void Window::openPackFile(const QString& startPath, const QString& filePath) {
	auto path = filePath;
	if (path.isEmpty()) {
		auto supportedExtensions = PackFile::getOpenableExtensions();
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
			worker->run(this, savePath, {
				.zip_compressionTypeOverride = this->packFileOptions.compressionType,
				.zip_compressionStrength = this->packFileOptions.compressionStrength,
			});
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
		worker->run(this, savePath, {
			.zip_compressionTypeOverride = this->packFileOptions.compressionType,
			.zip_compressionStrength = this->packFileOptions.compressionStrength,
		}, false);
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
	auto version = 0u;
	auto chunkSize = 0u;

	if (this->packFile->getType() == PackFileType::FPX) {
		if (auto fpx = dynamic_cast<FPX*>(this->packFile.get())) {
			chunkSize = fpx->getChunkSize();
		}
	} else if (this->packFile->getType() == PackFileType::VPK) {
		if (auto vpk = dynamic_cast<VPK*>(this->packFile.get())) {
			version = vpk->getVersion();
			chunkSize = vpk->getChunkSize();
		}
	}

	auto options = PackFileOptionsDialog::getForEdit(this->packFile->getType(), {
		.compressionType = this->packFileOptions.compressionType,
		.compressionStrength = this->packFileOptions.compressionStrength,
		.vpk_version = version,
		.vpk_chunkSize = chunkSize,
	}, this);
	if (!options) {
		return;
	}

	this->packFileOptions.compressionType = options->compressionType;
	this->packFileOptions.compressionStrength = options->compressionStrength;

	if (this->packFile->getType() == PackFileType::FPX) {
		if (auto fpx = dynamic_cast<FPX*>(this->packFile.get())) {
			fpx->setChunkSize(options->vpk_chunkSize);
		}
	} else if (this->packFile->getType() == PackFileType::FPX) {
		if (auto vpk = dynamic_cast<VPK*>(this->packFile.get())) {
			vpk->setVersion(options->vpk_version);
			vpk->setChunkSize(options->vpk_chunkSize);
		}
	}

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
	// Get file data
	auto entry = this->packFile->findEntry(oldPath.toLocal8Bit().constData());
	auto data = this->packFile->readEntry(oldPath.toLocal8Bit().constData());
	if (!entry || !data) {
		QMessageBox::critical(this, tr("Error"), tr("Unable to edit file at \"%1\": could not read file data!").arg(oldPath));
		return;
	}

	// Load existing properties
	EntryCompressionType compressionType = EntryCompressionType::NO_COMPRESS;
	int16_t compressionStrength = 5;
	if (this->packFile->getType() == PackFileType::BSP || this->packFile->getType() == PackFileType::ZIP) {
		if (auto* zip = dynamic_cast<ZIP*>(this->packFile.get())) {
			compressionType = zip->getEntryCompressionType(oldPath.toLocal8Bit().constData());
			compressionStrength = zip->getEntryCompressionStrength(oldPath.toLocal8Bit().constData());
		}
	}

	// Get new properties
	const auto options = EntryOptionsDialog::getEntryOptions(true, false, oldPath, this->packFile->getType(), {
		.zip_compressionType = compressionType,
		.zip_compressionStrength = compressionStrength,
		.vpk_saveToDirectory = entry->archiveIndex == VPK_DIR_INDEX,
		.vpk_preloadBytes = static_cast<uint32_t>(entry->extraData.size()),
	}, this);
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

void Window::editFileContents(const QString& path, std::vector<std::byte> data) {
	auto entry = this->packFile->findEntry(path.toLocal8Bit().constData());
	if (!entry) {
		return;
	}

	// Load existing properties
	EntryCompressionType compressionType = EntryCompressionType::NO_COMPRESS;
	int16_t compressionStrength = 5;
	if (this->packFile->getType() == PackFileType::BSP || this->packFile->getType() == PackFileType::ZIP) {
		if (auto* zip = dynamic_cast<ZIP*>(this->packFile.get())) {
			compressionType = zip->getEntryCompressionType(path.toLocal8Bit().constData());
			compressionStrength = zip->getEntryCompressionStrength(path.toLocal8Bit().constData());
		}
	}

	this->packFile->removeEntry(path.toLocal8Bit().constData());
	this->packFile->addEntry(path.toLocal8Bit().constData(), std::move(data), {
		.zip_compressionType = compressionType,
		.zip_compressionStrength = compressionStrength,
		.vpk_saveToDirectory = entry->archiveIndex == VPK_DIR_INDEX,
		.vpk_preloadBytes = static_cast<uint32_t>(entry->extraData.size()),
	});
	this->markModified(true);
}

void Window::editFileContents(const QString& path, const QString& data) {
	auto entry = this->packFile->findEntry(path.toLocal8Bit().constData());
	if (!entry) {
		return;
	}
	auto byteData = data.toLocal8Bit();

	// Load existing properties
	EntryCompressionType compressionType = EntryCompressionType::NO_COMPRESS;
	int16_t compressionStrength = 5;
	if (this->packFile->getType() == PackFileType::BSP || this->packFile->getType() == PackFileType::ZIP) {
		if (auto* zip = dynamic_cast<ZIP*>(this->packFile.get())) {
			compressionType = zip->getEntryCompressionType(path.toLocal8Bit().constData());
			compressionStrength = zip->getEntryCompressionStrength(path.toLocal8Bit().constData());
		}
	}

	this->packFile->removeEntry(path.toLocal8Bit().constData());
	this->packFile->addEntry(path.toLocal8Bit().constData(), std::vector<std::byte>{
		reinterpret_cast<std::byte*>(byteData.data()),
		reinterpret_cast<std::byte*>(byteData.data()) + byteData.size(),
	}, {
		.zip_compressionType = compressionType,
		.zip_compressionStrength = compressionStrength,
		.vpk_saveToDirectory = entry->archiveIndex == VPK_DIR_INDEX,
		.vpk_preloadBytes = static_cast<uint32_t>(entry->extraData.size()),
	});
	this->markModified(true);
}

void Window::encryptFile(const QString& path) {
	auto entry = this->packFile->findEntry(path.toLocal8Bit().constData());
	if (!entry) {
		return;
	}

	auto data = VICEDialog::encrypt(this, path, this);
	if (!data) {
		return;
	}

	// Load existing properties
	EntryCompressionType compressionType = EntryCompressionType::NO_COMPRESS;
	int16_t compressionStrength = 5;
	if (this->packFile->getType() == PackFileType::BSP || this->packFile->getType() == PackFileType::ZIP) {
		if (auto* zip = dynamic_cast<ZIP*>(this->packFile.get())) {
			compressionType = zip->getEntryCompressionType(path.toLocal8Bit().constData());
			compressionStrength = zip->getEntryCompressionStrength(path.toLocal8Bit().constData());
		}
	}

	auto newPath = path.sliced(0, path.length() - 4) + (path.sliced(path.length() - 4) == ".txt" ? ".ctx" : ".nuc");
	this->requestEntryRemoval(path);
	this->requestEntryRemoval(newPath);

	this->packFile->addEntry(newPath.toLocal8Bit().constData(), std::move(data.value()), {
		.zip_compressionType = compressionType,
		.zip_compressionStrength = compressionStrength,
		.vpk_saveToDirectory = entry->archiveIndex == VPK_DIR_INDEX,
		.vpk_preloadBytes = static_cast<uint32_t>(entry->extraData.size()),
	});
	this->entryTree->addEntry(newPath);
	this->fileViewer->addEntry(*this->packFile, newPath);
	this->markModified(true);
}

void Window::decryptFile(const QString& path) {
	auto entry = this->packFile->findEntry(path.toLocal8Bit().constData());
	if (!entry) {
		return;
	}

	auto data = VICEDialog::decrypt(this, path, this);
	if (!data) {
		return;
	}

	// Load existing properties
	EntryCompressionType compressionType = EntryCompressionType::NO_COMPRESS;
	int16_t compressionStrength = 5;
	if (this->packFile->getType() == PackFileType::BSP || this->packFile->getType() == PackFileType::ZIP) {
		if (auto* zip = dynamic_cast<ZIP*>(this->packFile.get())) {
			compressionType = zip->getEntryCompressionType(path.toLocal8Bit().constData());
			compressionStrength = zip->getEntryCompressionStrength(path.toLocal8Bit().constData());
		}
	}

	auto newPath = path.sliced(0, path.length() - 4) + (path.sliced(path.length() - 4) == ".ctx" ? ".txt" : ".nut");
	this->requestEntryRemoval(path);
	this->requestEntryRemoval(newPath);

	this->packFile->addEntry(newPath.toLocal8Bit().constData(), std::move(data.value()), {
		.zip_compressionType = compressionType,
		.zip_compressionStrength = compressionStrength,
		.vpk_saveToDirectory = entry->archiveIndex == VPK_DIR_INDEX,
		.vpk_preloadBytes = static_cast<uint32_t>(entry->extraData.size()),
	});
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

	// todo: use the new PackFile::renameDirectory method it'll be way faster

	QList<std::pair<std::string, Entry>> entriesToRename;
	this->packFile->runForAllEntries([&oldPath, &entriesToRename](const std::string& path, const Entry& entry) {
		if (path.starts_with((oldPath + '/').toLocal8Bit().constData())) {
			entriesToRename.emplace_back(path, entry);
		}
	});

	QProgressDialog progressDialog(tr("Renaming folder... Aborting this process will not roll back changes made so far."), tr("Abort"), 0, entriesToRename.size(), this);
	progressDialog.setWindowTitle(tr("Rename Folder"));
	progressDialog.setWindowModality(Qt::WindowModal);
	for (const auto& [path, entry] : entriesToRename) {
		if (progressDialog.wasCanceled()) {
			break;
		}

		// Get data
		auto entryData = this->packFile->readEntry(path);
		if (!entryData) {
			continue;
		}

		// Load existing properties
		EntryCompressionType compressionType = EntryCompressionType::NO_COMPRESS;
		int16_t compressionStrength = 5;
		if (this->packFile->getType() == PackFileType::BSP || this->packFile->getType() == PackFileType::ZIP) {
			if (auto* zip = dynamic_cast<ZIP*>(this->packFile.get())) {
				compressionType = zip->getEntryCompressionType(oldPath.toLocal8Bit().constData());
				compressionStrength = zip->getEntryCompressionStrength(oldPath.toLocal8Bit().constData());
			}
		}

		// Remove file
		this->requestEntryRemoval(path.c_str());

		// Calculate new path
		QString newEntryPath = newPath + path.substr(oldPath.length()).c_str();

		// Add new file with the same info and data at the new path
		this->packFile->addEntry(newEntryPath.toLocal8Bit().constData(), std::move(entryData.value()), {
			.zip_compressionType = compressionType,
			.zip_compressionStrength = compressionStrength,
			.vpk_saveToDirectory = entry.archiveIndex == VPK_DIR_INDEX,
			.vpk_preloadBytes = static_cast<unsigned int>(entry.extraData.size()),
		});
		this->entryTree->addEntry(newEntryPath);
		this->fileViewer->addEntry(*this->packFile, newEntryPath);

		progressDialog.setValue(progressDialog.value() + 1);
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
	return this->packFile->readEntry(path.toLocal8Bit().constData());
}

std::optional<QString> Window::readTextEntry(const QString& path) const {
	auto binData = this->packFile->readEntry(path.toLocal8Bit().constData());
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

void Window::extractFile(const QString& entryPath, QString savePath) {
	if (savePath.isEmpty()) {
		QString filter;
		if (auto index = entryPath.lastIndexOf('.'); index >= 0) {
			auto fileExt = entryPath.sliced(index); // ".ext"
			auto fileExtPretty = fileExt.toUpper();
			fileExtPretty.remove('.');

			filter = fileExtPretty + " (*" + fileExt + ");;All files (*.*)";
		}
		savePath = QFileDialog::getSaveFileName(this, tr("Extract as..."), entryPath, filter);
	}
	if (savePath.isEmpty()) {
		return;
	}
	this->writeEntryToFile(entryPath, savePath);
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
	this->packFile->runForAllEntries([&predicate, &progressBarMax](const std::string& path, const Entry& entry) {
		if (predicate(QString(path.c_str()))) {
			progressBarMax++;
		}
	});

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
	QObject::connect(worker, &ExtractPackFileWorker::taskFinished, this, [this, saveDir](bool noneFailed) {
		// Kill thread
		this->extractPackFileWorkerThread->quit();
		this->extractPackFileWorkerThread->wait();
		delete this->extractPackFileWorkerThread;
		this->extractPackFileWorkerThread = nullptr;

		this->freezeActions(false);

		this->resetStatusBar();

		if (!noneFailed) {
			QMessageBox::critical(this, tr("Error"), tr(R"(Failed to write some or all files to "%1". Please ensure that a game or another application is not using the file, and that you have sufficient permissions to write to the save location.)").arg(saveDir));
		}
	});
	this->extractPackFileWorkerThread->start();
}

void Window::extractDir(const QString& path, const QString& saveDir) {
	this->extractFilesIf([path](const QString& entryPath) { return entryPath.startsWith(path + '/'); }, saveDir);
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

void Window::freezeActions(bool freeze, bool freezeCreationActions, bool freezeFileViewer) const {
	this->createEmptyMenu->setDisabled(freeze && freezeCreationActions);
	this->createFromDirMenu->setDisabled(freeze && freezeCreationActions);
	this->openAction->setDisabled(freeze && freezeCreationActions);
	this->openDirAction->setDisabled(freeze && freezeCreationActions);
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
	this->fileViewer->setDisabled(freeze && freezeFileViewer);
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

bool Window::isModified() const {
	return this->modified;
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
	if (!event->mimeData()->hasUrls()) {
		return;
	}

	for (auto& url : event->mimeData()->urls()) {
		if (!url.isLocalFile()) {
			return;
		}
	}

	if (this->fileViewer->isDirPreviewVisible()) {
		// If file viewer is open, it'll just add the files to the open pack file
		event->acceptProposedAction();
	} else if (!this->packFile) {
		// If we don't have a pack file open, and the path is a pack file, we can load it instead
		auto path = event->mimeData()->urls()[0].path();
		auto fileTypes = PackFile::getOpenableExtensions();
		if (std::any_of(fileTypes.begin(), fileTypes.end(), [&path](const std::string& extension) { return path.endsWith(extension.c_str()); })) {
			event->acceptProposedAction();
		}
	}
}

void Window::dropEvent(QDropEvent* event) {
	if (!event->mimeData()->hasUrls()) {
		return;
	}

	if (!this->packFile) {
		// If we don't have a pack file open, try loading the one given (extension's already been verified)
		this->loadPackFile(event->mimeData()->urls()[0].toLocalFile());
		return;
	}

	if (!this->dropEnabled || !this->fileViewer->isDirPreviewVisible()) {
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

	// Write location and sizing
	auto* settings = Options::getOptions();
	settings->beginGroup("main_window");
	settings->setValue("geometry", this->saveGeometry());
	settings->setValue("state", this->saveState());
	settings->setValue("maximized", this->isMaximized());
	if (!this->isMaximized()) {
		settings->setValue("position", this->pos());
		settings->setValue("size", this->size());
	}
	settings->endGroup();

	event->accept();
}

bool Window::loadDir(const QString& path) {
	return this->loadPackFile(path, Folder::open(path.toLocal8Bit().constData()));
}

bool Window::loadPackFile(const QString& path) {
	return this->loadPackFile(path, PackFile::open(path.toLocal8Bit().constData()));
}

bool Window::loadPackFile(const QString& path, std::unique_ptr<vpkpp::PackFile>&& newPackFile) {
	if (!this->clearContents()) {
		return false;
	}
	this->freezeActions(true);

	auto recentPaths = Options::get<QStringList>(STR_OPEN_RECENT);

	QString fixedPath = QDir(path).absolutePath();
	fixedPath.replace('\\', '/');

	this->packFile = std::move(newPackFile);
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

	// Reset properties that we care about
	this->packFileOptions.compressionType = EntryCompressionType::NO_COMPRESS;
	this->packFileOptions.compressionStrength = 5;

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

	this->entryTree->loadPackFile(*this->packFile, this->statusProgressBar, [this] {
		this->freezeActions(false);
		this->freezeModifyActions(this->isReadOnly());

		this->resetStatusBar();
	});

	this->fileViewer->setReadOnly(this->packFile->isReadOnly());

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
			if (std::filesystem::is_directory(path.toLocal8Bit().constData())) {
				this->loadDir(path);
			} else {
				this->loadPackFile(path);
			}
		});
	}
	this->openRecentMenu->addSeparator();
	this->openRecentMenu->addAction(tr("Clear"), [this] {
		Options::set(STR_OPEN_RECENT, QStringList{});
		this->rebuildOpenRecentMenu({});
	});
}

bool Window::writeEntryToFile(const QString& entryPath, const QString& filepath) {
	return this->packFile->extractEntry(entryPath.toLocal8Bit().constData(), filepath.toLocal8Bit().constData());
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

void SavePackFileWorker::run(Window* window, const QString& savePath, BakeOptions options, bool async) {
	std::unique_ptr<QEventLoop> loop;
	if (!async) {
		loop = std::make_unique<QEventLoop>();
	}
	int currentEntry = 0;
	bool success = window->packFile->bake(savePath.toLocal8Bit().constData(), options, [this, loop_=loop.get(), &currentEntry](const std::string&, const Entry&) {
		emit this->progressUpdated(++currentEntry);
		if (loop_) {
			loop_->processEvents();
		}
	});
	emit this->taskFinished(success);
}

void ExtractPackFileWorker::run(Window* window, const QString& saveDir, const std::function<bool(const QString&)>& predicate) {
	int currentEntry = 0;
	bool out = window->packFile->extractAll(saveDir.toLocal8Bit().constData(), [this, &predicate, &currentEntry](const std::string& path, const Entry& entry) -> bool {
		emit this->progressUpdated(++currentEntry);
		return predicate(path.c_str());
	}, false);
	emit this->taskFinished(out);
}

void ScanSteamGamesWorker::run() {
	QList<std::tuple<QString, QIcon, QDir>> sourceGames;

	if (Options::get<bool>(OPT_DISABLE_STEAM_SCANNER)) {
		emit this->taskFinished(sourceGames);
		return;
	}

	Steam steam;
	if (!steam) {
		emit this->taskFinished(sourceGames);
		return;
	}

	// Add Steam games
	for (auto appID : steam.getInstalledApps()) {
		if (!steam.isAppUsingSourceEngine(appID) && !steam.isAppUsingSource2Engine(appID)) {
			continue;
		}
		auto iconPath = steam.getAppIconPath(appID);
		sourceGames.emplace_back(steam.getAppName(appID).data(), iconPath.empty() ? QIcon(":/icons/missing_app.png") : QIcon(iconPath.c_str()), steam.getAppInstallDir(appID).c_str());
	}

	// Add mods in the sourcemods directory
	for (const auto& modDir : std::filesystem::directory_iterator{steam.getSourceModDir()}) {
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

		KV1 gameInfoRoot{gameInfoData};
		if (gameInfoRoot.isInvalid()) {
			continue;
		}
		const auto& gameInfo = gameInfoRoot["GameInfo"];
		if (gameInfo.isInvalid()) {
			continue;
		}
		const auto& gameInfoName = gameInfo["game"];
		const auto& gameInfoIconPath = gameInfo["icon"];

		std::string modName;
		if (!gameInfoName.isInvalid()) {
			modName = gameInfoName.getValue();
		} else {
			modName = std::filesystem::path{gameInfoPath}.parent_path().filename().string();
		}

		std::string modIconPath;
		if (!gameInfoIconPath.isInvalid()) {
			if (auto modIconBigPath = (modDir.path() / (std::string{gameInfoIconPath.getValue()} + "_big.tga")); std::filesystem::exists(modIconBigPath)) {
				modIconPath = modIconBigPath.string();
			} else if (auto modIconRegularPath = (modDir.path() / (std::string{gameInfoIconPath.getValue()} + ".tga")); std::filesystem::exists(modIconRegularPath)) {
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
