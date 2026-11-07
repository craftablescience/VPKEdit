#include "VMainWindow.hpp"

#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QScreen>
#include <QSplitter>
#include <QStyleFactory>
#include <QTimer>
#include <QVBoxLayout>

#include "Config.hpp"

#include "dialogs/VCreditsDialog.hpp"
#include "dialogs/VPluginDialog.hpp"
#include "utility/VDiscordPresence.hpp"
#include "utility/VSettings.hpp"
#include "widgets/previews/VPreviewDirectoryParent.hpp"
#include "widgets/VEntryTreeView.hpp"
#include "widgets/VNavigationBar.hpp"
#include "widgets/VPreviewParent.hpp"

VMainWindow* g_MainWindow = nullptr;

VMainWindow::VMainWindow() {
	this->setWindowTitle(PROJECT_TITLE "[*]");
	this->setWindowIcon(QIcon(":/logo.png"));
	this->resize(this->screen()->availableGeometry().size() * 0.7);

	// --- Settings helper ---

	const auto showRestartWarning = [this] {
		static bool shownRestartMessage = false;
		if (!shownRestartMessage) {
			QMessageBox::warning(this, tr("Restart Required"), tr("The application must be restarted for these settings to take effect."));
			shownRestartMessage = true;
		}
	};

	// --- File menu ---

	auto* fileMenu = this->menuBar()->addMenu(tr("&File"));

	fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DirIcon), tr("&Open"), Qt::CTRL | Qt::Key_O, [this] {
		// todo: proper extensions
		if (const auto files = QFileDialog::getOpenFileNames(this, tr("Open Files"), VSettings::get<QString>(VSettings::STR_DEFAULT_OPEN_OR_SAVE_DIALOG_DIR), QString{"%1 (*.bsp *.gma *.vpk *.zip);;%2 (*)"}.arg("Pack File Formats", "All Files")); !files.empty()) {
			VSettings::set(VSettings::STR_DEFAULT_OPEN_OR_SAVE_DIALOG_DIR, QFileInfo{files.last()}.canonicalPath());
			for (const auto& file : files) {
				this->addPackFile(file);
			}
		}
	});

	fileMenu->addSeparator();

	fileMenu->addAction(QIcon{":/icons/button_kofi.png"}, tr("&Donate"), [] {
		QDesktopServices::openUrl({"https://ko-fi.com/craftablescience"});
	});

	fileMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogCancelButton), tr("&Quit"), QKeySequence::Quit, [this] {
		this->close();
	});

	// --- Edit menu ---

	auto* editMenu = this->menuBar()->addMenu(tr("&Edit"));

	editMenu->addAction(this->style()->standardIcon(QStyle::SP_ComputerIcon), tr("View Active Plugins…"), [this] {
		QMap<QString, QList<VPluginLoader::Metadata>> pluginMetadata;
		pluginMetadata[tr("Thumbnails")] = this->entryTreeView->thumbnailProvider()->thumbnailPluginLoader()->getPluginMetadata();
		VPluginDialog::showPlugins(pluginMetadata);
	});

	// --- Options menu ---

	auto* optionsMenu = this->menuBar()->addMenu(tr("&Options"));

	auto* generalMenu = optionsMenu->addMenu(QIcon{":/logo.png"}, tr("&General"));

	auto* optionRaiseToTopOpeningFileAction = generalMenu->addAction(tr("Raise When Opening New File"), std::bind_front(&VSettings::invert, VSettings::BOOL_RAISE_TO_TOP_OPENING_FILE));
	optionRaiseToTopOpeningFileAction->setCheckable(true);
	optionRaiseToTopOpeningFileAction->setChecked(VSettings::get<bool>(VSettings::BOOL_RAISE_TO_TOP_OPENING_FILE));

	auto* optionAllowMultipleAppInstancesAction = generalMenu->addAction(tr("Allow Multiple Application Instances"), [showRestartWarning] {
		showRestartWarning();
		VSettings::invert(VSettings::BOOL_ALLOW_MULTIPLE_APP_INSTANCES);
	});
	optionAllowMultipleAppInstancesAction->setCheckable(true);
	optionAllowMultipleAppInstancesAction->setChecked(VSettings::get<bool>(VSettings::BOOL_ALLOW_MULTIPLE_APP_INSTANCES));

	static constexpr auto setupDiscordRichPresence = [] {
		auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
#ifdef Q_OS_WIN
		std::tm currentTimeVal{};
		localtime_s(&currentTimeVal, &time);
		const auto* currentTime = &currentTimeVal;
#else
		const auto* currentTime = std::localtime(&time);
#endif
		if (currentTime->tm_mon == 3 && currentTime->tm_mday == 1) {
			// It's April 1st, you know what that means!
			VDiscordPresence::init("1232981268472533032");
			VDiscordPresence::setDetails("Customizing character...");
			VDiscordPresence::setState("Ponyville Train Station");
		} else {
			VDiscordPresence::init("1222285763459158056");
			VDiscordPresence::setState("Editing an archive file");
			VDiscordPresence::setLargeImageText(PROJECT_TITLE);
		}
		VDiscordPresence::setLargeImage("icon");
		VDiscordPresence::setStartTimestamp(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
		VDiscordPresence::setTopButton({ .name = "View on GitHub", .url = PROJECT_GITHUB_URL });
	};
	auto* discordEnableAction = generalMenu->addAction(tr("Enable Rich Presence"), [] {
		VSettings::invert(VSettings::BOOL_ENABLE_DISCORD_RICH_PRESENCE);
		if (VSettings::get<bool>(VSettings::BOOL_ENABLE_DISCORD_RICH_PRESENCE)) {
			setupDiscordRichPresence();
		} else {
			VDiscordPresence::shutdown();
		}
	});
	discordEnableAction->setCheckable(true);
	discordEnableAction->setChecked(VSettings::get<bool>(VSettings::BOOL_ENABLE_DISCORD_RICH_PRESENCE));
	if (VSettings::get<bool>(VSettings::BOOL_ENABLE_DISCORD_RICH_PRESENCE)) {
		setupDiscordRichPresence();
	}

	auto* entryListMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("&Entry Tree"));

	auto* entryListMenuHideIconsAction = entryListMenu->addAction(tr("Hide Icons"), std::bind_front(&VSettings::invert, VSettings::BOOL_ENTRY_TREE_HIDE_ICONS));
	entryListMenuHideIconsAction->setCheckable(true);
	entryListMenuHideIconsAction->setChecked(VSettings::get<bool>(VSettings::BOOL_ENTRY_TREE_HIDE_ICONS));

	auto* languageMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("&Language"));
	auto* languageMenuGroup = new QActionGroup{languageMenu};
	languageMenuGroup->setExclusive(true);
	const QList<QPair<QString, QString>> languageToLocaleMapping{
		{tr("System Language"), ""},
		{"", ""}, // Separator
		{u8"English", "en"},
	};
	for (const auto& [language, locale] : languageToLocaleMapping) {
		if (language.isEmpty() && locale.isEmpty()) {
			languageMenu->addSeparator();
			continue;
		}
		auto* action = languageMenu->addAction(language, [showRestartWarning, locale_=locale] {
			showRestartWarning();
			VSettings::set(VSettings::STR_LANGUAGE_OVERRIDE, locale_);
		});
		action->setCheckable(true);
		action->setChecked(locale == VSettings::get<QString>(VSettings::STR_LANGUAGE_OVERRIDE));
		languageMenuGroup->addAction(action);
	}

	auto* themeMenu = optionsMenu->addMenu(this->style()->standardIcon(QStyle::SP_DesktopIcon), tr("&Theme"));
	auto* themeMenuGroup = new QActionGroup{themeMenu};
	themeMenuGroup->setExclusive(true);
	for (const auto& themeName : QStyleFactory::keys()) {
		auto* action = themeMenu->addAction(themeName, [this, themeName] {
			QApplication::setStyle(themeName);
			VSettings::set(VSettings::STR_STYLE, themeName);
			emit this->themeUpdated();
		});
		action->setCheckable(true);
		if (themeName.toLower() == VSettings::get<QString>(VSettings::STR_STYLE).toLower()) {
			action->setChecked(true);
		}
		themeMenuGroup->addAction(action);
	}

	// --- Help menu ---

	auto* helpMenu = this->menuBar()->addMenu(tr("&Help"));

	helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("Credi&ts"), Qt::Key_F1, [this] {
		VCreditsDialog::showCredits(this);
	});

	helpMenu->addSeparator();

	helpMenu->addAction(this->style()->standardIcon(QStyle::SP_MessageBoxInformation), tr("Report an &Issue"), [] {
		QDesktopServices::openUrl({PROJECT_GITHUB_URL "/issues/new"});
	});

	helpMenu->addAction(this->style()->standardIcon(QStyle::SP_MessageBoxInformation), tr("Request a &Feature"), [] {
		QDesktopServices::openUrl({PROJECT_GITHUB_URL "/issues/new"});
	});

	// --- Widgets ---

	auto* splitter = new QSplitter{this};
	this->setCentralWidget(splitter);

	auto* entryTreeParent = new QWidget{this};
	auto* entryTreeParentLayout = new QVBoxLayout{entryTreeParent};

	this->entryTreeSearch = new QLineEdit{this};
	this->entryTreeSearch->setPlaceholderText(tr("Search..."));
	entryTreeParentLayout->addWidget(this->entryTreeSearch);

	this->entryTreeView = new VEntryTreeView{this};
	entryTreeParentLayout->addWidget(this->entryTreeView);

	splitter->addWidget(entryTreeParent);

	auto* previewParentParent = new QWidget{this};
	auto* previewParentParentLayout = new QVBoxLayout{previewParentParent};

	this->navigationBar = new VNavigationBar{this};
	this->navigationBar->setFixedHeight(this->entryTreeSearch->height());
	previewParentParentLayout->addWidget(this->navigationBar);

	this->previewParent = new VPreviewParent{this};
	previewParentParentLayout->addWidget(this->previewParent);

	this->navigationBar->setContentsMargins(previewParentParentLayout->contentsMargins().left(), this->navigationBar->contentsMargins().top(), previewParentParentLayout->contentsMargins().right(), this->navigationBar->contentsMargins().bottom());
	previewParentParentLayout->setContentsMargins(0, previewParentParentLayout->contentsMargins().top(), 0, 0);

	splitter->addWidget(previewParentParent);

	splitter->setSizes({this->width() * 1 / 4, this->width() * 3 / 4});

	// --- Logic ---

	connect(this->entryTreeView, &VEntryTreeView::dirSelected, this->navigationBar, &VNavigationBar::onDirSelected);
	connect(this->entryTreeView, &VEntryTreeView::dirSelected, this->previewParent, &VPreviewParent::onDirSelected);

	connect(this->entryTreeView, &VEntryTreeView::fileSelected, this->navigationBar, &VNavigationBar::onFileSelected);
	connect(this->entryTreeView, &VEntryTreeView::fileSelected, this->previewParent, &VPreviewParent::onFileSelected);

	connect(this->entryTreeView->thumbnailProvider(), &VEntryTreeThumbnailProvider::processedThumbnail, this->previewParent->previewDirectoryParent(), &VPreviewDirectoryParent::onProcessedThumbnail);

	connect(this->navigationBar, &VNavigationBar::dirViewModeUpdated, this->previewParent->previewDirectoryParent(), &VPreviewDirectoryParent::onDirViewModeUpdated);

	//connect(this->navigationBar, &VNavigationBar::pathModified, this->entryTreeView, &VEntryTreeView::selectEntry);

	connect(this, &VMainWindow::themeUpdated, this->entryTreeView, &VEntryTreeView::onThemeUpdated);

	// --- Finalize ---

	auto* discordUpdateTimer = new QTimer{this};
	connect(discordUpdateTimer, &QTimer::timeout, this, &VDiscordPresence::update);
	discordUpdateTimer->start(20);

	this->setAcceptDrops(true);

	// todo: testing
	this->addPackFile("/home/lxlewis/Downloads/cfg.zip");
	//this->addPackFile("/home/lxlewis/Downloads/Reduced_CSDK_12/game/core/shaders_vulkan_dir.vpk");
	this->addPackFile("/home/lxlewis/Steam/steamapps/common/Half-Life 2/hl2/hl2_textures_dir.vpk");
	//this->addPackFile("/home/lxlewis/Steam/steamapps/common/Counter-Strike Global Offensive/game/csgo/pak01_dir.vpk");
}

void VMainWindow::addPackFile(const QString& path) const {
	this->entryTreeView->addPackFile(path);
}

void VMainWindow::dragEnterEvent(QDragEnterEvent* event) {
	if (!event->mimeData()->hasUrls()) {
		return;
	}
	for (const auto& url : event->mimeData()->urls()) {
		if (!(
			url.isLocalFile()
			// todo: check extension
		)) {
			return;
		}
	}
	event->acceptProposedAction();
}

void VMainWindow::dropEvent(QDropEvent* event) {
	for (const auto& url : event->mimeData()->urls()) {
		this->addPackFile(url.toLocalFile());
	}
	event->acceptProposedAction();
}
