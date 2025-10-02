#include "FileViewer.h"

#include <filesystem>
#include <ranges>

#include <QApplication>
#include <QJsonArray>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPluginLoader>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>

#include "Config.h"
#include "previews/DirPreview.h"
#include "previews/EmptyPreview.h"
#include "previews/InfoPreview.h"
#include "previews/TextPreview.h"
#include "previews/TexturePreview.h"
#include "utility/ThemedIcon.h"
#include "Window.h"

using namespace vpkpp;

static QString stripSlashes(QString input) {
	while (input.startsWith('/')) {
		input = input.sliced(1);
	}
	while (input.endsWith('/')) {
		if (input.length() <= 1) {
			input = "";
		} else {
			input = input.sliced(0, input.length() - 1);
		}
	}
	return input;
}

/*
 * NavBar holds a list of places visited, uses pathChanged signal to tell FileViewer to go between them
 * Back button - goes back in the list
 * Next button - goes forward in the list
 * Up button - strips the directory/file off the end of the path
 * Home button - go to the root directory
 * Current Path - set the path explicitly, if valid fire pathChanged, otherwise reset the text to last valid path
 */
NavBar::NavBar(Window* window_, QWidget* parent)
		: QWidget(parent)
		, window(window_)
		, historyIndex(0) {
	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	this->backButton = new QToolButton(this);
	this->backButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	this->backButtonAction = new QAction(this->backButton);
	this->backButtonAction->setShortcut(Qt::ALT | Qt::Key_Left);
	this->backButton->setDefaultAction(this->backButtonAction);
	QObject::connect(this->backButton, &QToolButton::triggered, this, &NavBar::navigateBack);
	layout->addWidget(this->backButton);

	this->nextButton = new QToolButton(this);
	this->nextButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	this->nextButtonAction = new QAction(this->nextButton);
	this->nextButtonAction->setShortcut(Qt::ALT | Qt::Key_Right);
	this->nextButton->setDefaultAction(this->nextButtonAction);
	QObject::connect(this->nextButton, &QToolButton::triggered, this, &NavBar::navigateNext);
	layout->addWidget(this->nextButton);

	this->upButton = new QToolButton(this);
	this->upButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	this->upButtonAction = new QAction(this->upButton);
	this->upButtonAction->setShortcuts({Qt::ALT | Qt::Key_Up, Qt::Key_Backspace});
	this->upButton->setDefaultAction(this->upButtonAction);
	QObject::connect(this->upButton, &QToolButton::triggered, this, &NavBar::navigateUp);
	layout->addWidget(this->upButton);

	this->homeButton = new QToolButton(this);
	this->homeButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	this->homeButtonAction = new QAction(this->homeButton);
	this->homeButtonAction->setShortcut(Qt::Key_Home);
	this->homeButton->setDefaultAction(this->homeButtonAction);
	QObject::connect(this->homeButton, &QToolButton::triggered, this, &NavBar::navigateHome);
	layout->addWidget(this->homeButton);

	this->currentPath = new QLineEdit(this);
	this->currentPath->setPlaceholderText(tr("Navigate..."));
	QObject::connect(this->currentPath, &QLineEdit::editingFinished, this, &NavBar::navigatePath);
	layout->addWidget(this->currentPath);

	QObject::connect(this->window, &Window::themeUpdated, this, &NavBar::resetButtonIcons, Qt::QueuedConnection);
}

QString NavBar::path() const {
	return this->currentPath->text();
}

void NavBar::setPath(const QString& newPath) {
	this->currentPath->setText(newPath);
	this->processPathChanged(newPath, true, false);
}

void NavBar::navigateBack() {
	auto oldIndex = this->historyIndex;
	do {
		if (this->historyIndex == 0) {
			return;
		}
		this->historyIndex--;
	} while (!this->window->hasEntry(this->history.at(this->historyIndex)));

	if (oldIndex != this->historyIndex) {
		this->processPathChanged(this->history.at(this->historyIndex), false);
	}
}

void NavBar::navigateNext() {
	auto oldIndex = this->historyIndex;
	do {
		if (this->historyIndex == this->history.size() - 1) {
			return;
		}
		this->historyIndex++;
	} while (!this->window->hasEntry(this->history.at(this->historyIndex)));

	if (oldIndex != this->historyIndex) {
		this->processPathChanged(this->history.at(this->historyIndex), false);
	}
}

void NavBar::navigateUp() {
	auto path = this->history.at(this->historyIndex);
	auto index = path.lastIndexOf('/');
	if (index < 0) {
		return this->navigateHome();
	}
	path = path.sliced(0, index);
	this->processPathChanged(path);
}

void NavBar::navigateHome() {
	this->processPathChanged("");
}

void NavBar::navigatePath() {
	auto newPath = stripSlashes(this->currentPath->text());
	if (!this->window->hasEntry(newPath)) {
		this->currentPath->setText(this->history.at(this->historyIndex));
		return;
	}
	this->processPathChanged(newPath);
}

void NavBar::clearContents(bool resetHistory) {
	QTimer::singleShot(0, this, &NavBar::resetButtonIcons);
	this->currentPath->clear();

	if (resetHistory) {
		this->history.clear();
		this->history.push_back(""); // root
		this->historyIndex = 0;

		this->backButton->setDisabled(true);
		this->nextButton->setDisabled(true);
	}
}

void NavBar::resetButtonIcons() const {
	this->backButtonAction->setIcon(ThemedIcon::get(this, ":/icons/left.png", QPalette::ColorRole::ButtonText));
	this->nextButtonAction->setIcon(ThemedIcon::get(this, ":/icons/right.png", QPalette::ColorRole::ButtonText));
	this->upButtonAction->setIcon(ThemedIcon::get(this, ":/icons/up.png", QPalette::ColorRole::ButtonText));
	this->homeButtonAction->setIcon(ThemedIcon::get(this, ":/icons/home.png", QPalette::ColorRole::ButtonText));
}

void NavBar::processPathChanged(const QString& newPath, bool addToHistory, bool firePathChanged) {
	if (addToHistory && newPath != this->history.at(this->historyIndex)) {
		while (this->historyIndex < this->history.size() - 1) {
			this->history.pop_back();
		}
		this->history.push_back(newPath);
		this->historyIndex = static_cast<int>(this->history.size() - 1);

		while (this->history.size() > NavBar::NAVIGATION_HISTORY_LIMIT) {
			this->history.pop_front();
			this->historyIndex--;
		}
	}

	this->backButton->setDisabled(!this->historyIndex);
	this->nextButton->setDisabled(this->historyIndex == this->history.size() - 1);
	this->upButton->setDisabled(this->history.at(this->historyIndex) == "");
	this->homeButton->setDisabled(this->history.at(this->historyIndex) == "");

	QTimer::singleShot(0, this, &NavBar::resetButtonIcons);

	if (firePathChanged) {
		emit this->pathChanged(newPath);
	}
}

IVPKEditPreviewPlugin_V1_0_WindowAccess::IVPKEditPreviewPlugin_V1_0_WindowAccess(FileViewer* fileViewer_)
		: IVPKEditPreviewPlugin_V1_0_IWindowAccess(fileViewer_)
		, fileViewer(fileViewer_) {}

bool IVPKEditPreviewPlugin_V1_0_WindowAccess::hasEntry(const QString& entryPath) {
	return this->fileViewer->window->hasEntry(entryPath);
}

bool IVPKEditPreviewPlugin_V1_0_WindowAccess::readBinaryEntry(const QString& entryPath, QByteArray& data) {
	const auto file = this->fileViewer->window->readBinaryEntry(entryPath);
	if (!file) {
		return false;
	}
	data = QByteArray{reinterpret_cast<const char*>(file->data()), static_cast<qlonglong>(file->size())};
	return true;
}

bool IVPKEditPreviewPlugin_V1_0_WindowAccess::readTextEntry(const QString& entryPath, QString& data) {
	const auto file = this->fileViewer->window->readTextEntry(entryPath);
	if (!file) {
		return false;
	}
	data = *file;
	return true;
}

void IVPKEditPreviewPlugin_V1_0_WindowAccess::selectEntryInEntryTree(const QString& entryPath) {
	this->fileViewer->window->selectEntryInEntryTree(entryPath);
}

FileViewer::FileViewer(Window* window_, QWidget* parent)
		: QWidget(parent)
		, window(window_) {
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	this->navbar = new NavBar(this->window, this);
	QObject::connect(this->navbar, &NavBar::pathChanged, this, [this](const QString& newPath) {
		this->window->selectEntryInEntryTree(newPath);
	});
	layout->addWidget(this->navbar);

	this->packFileAccess_V1_0 = new IVPKEditPreviewPlugin_V1_0_WindowAccess{this};

	QStringList pluginLocations{QApplication::applicationDirPath()};
#if defined(_WIN32)
	for (const auto& path : QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)) {
		pluginLocations << path + "/" + QString{PROJECT_NAME.data()};
	}
#elif defined(__APPLE__)
	pluginLocations << QDir{"../PlugIns"}.absolutePath();
#elif defined(__linux__)
	pluginLocations << "/usr/" VPKEDIT_LIBDIR "/" + QString{PROJECT_NAME.data()};
	pluginLocations << QDir{"~/.local/" VPKEDIT_LIBDIR "/" + QString{PROJECT_NAME.data()}}.canonicalPath();
#endif
	qDebug() << pluginLocations;
	for (const QString& dirPath : pluginLocations) {
		for (const QDir dir{dirPath + "/previews"}; const QString& libraryName : dir.entryList(QDir::Files)) {
			auto* loader = new QPluginLoader{dir.absoluteFilePath(libraryName), this};
			if (auto* plugin = qobject_cast<IVPKEditPreviewPlugin_V1_0*>(loader->instance())) {
				if (!loader->metaData().contains("MetaData") || !loader->metaData().value("MetaData").isObject()) {
					continue;
				}
				this->previewPlugins.push_back(loader);
				plugin->initPlugin(this->packFileAccess_V1_0);
				plugin->initPreview(this);
				layout->addWidget(plugin->getPreview());
				QObject::connect(plugin, &IVPKEditPreviewPlugin_V1_0::showInfoPreview, this, &FileViewer::showInfoPreview);
				QObject::connect(plugin, &IVPKEditPreviewPlugin_V1_0::showGenericErrorPreview, this, &FileViewer::showGenericErrorPreview);
			} else {
				loader->deleteLater();
			}
		}
	}

	this->dirPreview = new DirPreview{this, this->window, this};
	layout->addWidget(this->dirPreview);

	this->emptyPreview = new EmptyPreview{this};
	layout->addWidget(this->emptyPreview);

	this->infoPreview = new InfoPreview{this};
	layout->addWidget(this->infoPreview);

	this->textPreview = new TextPreview{this, this->window, this};
	layout->addWidget(this->textPreview);

	this->texturePreview = new TexturePreview{this, this};
	layout->addWidget(this->texturePreview);

	this->clearContents(true);
}

void FileViewer::requestNavigateBack() const {
	this->navbar->navigateBack();
}

void FileViewer::requestNavigateNext() const {
	this->navbar->navigateNext();
}

void FileViewer::displayEntry(const QString& path, PackFile& packFile) {
	// Get extension
	std::filesystem::path helperPath(path.toLower().toLocal8Bit().constData());
	QString extension(helperPath.has_extension() ? helperPath.extension().string().c_str() : helperPath.stem().string().c_str());

	this->clearContents(false);
	this->navbar->setPath(path);

	// Check plugins first
	for (auto* pluginLoader : this->previewPlugins) {
		const auto& metadata = pluginLoader->metaData().value("MetaData").toObject();
		if (!metadata.contains("extensions") || !metadata.value("extensions").isArray()) {
			continue;
		}
		for (const auto extensionArray = metadata.value("extensions").toArray(); const auto& pluginExtension : extensionArray) {
			if (pluginExtension.isString() && pluginExtension.toString() == extension) {
				const auto binary = this->window->readBinaryEntry(path);
				if (!binary) {
					this->showFileLoadErrorPreview();
					return;
				}
				this->hideAllPreviews();
				auto* plugin = qobject_cast<IVPKEditPreviewPlugin_V1_0*>(pluginLoader->instance());
				plugin->getPreview()->show();
				plugin->setData(path, reinterpret_cast<const quint8*>(binary->data()), binary->size());
				return;
			}
		}
	}

	if (TexturePreview::EXTENSIONS_IMAGE.contains(extension)) {
		// Image
		auto binary = this->window->readBinaryEntry(path);
		if (!binary) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->hideAllPreviews();
		this->texturePreview->show();
		this->texturePreview->setImageData(*binary);
	} else if (TexturePreview::EXTENSIONS_SVG.contains(extension)) {
		// SVG
		auto binary = this->window->readBinaryEntry(path);
		if (!binary) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->hideAllPreviews();
		this->texturePreview->show();
		this->texturePreview->setSVGData(*binary);
	} else if (TexturePreview::EXTENSIONS_PPL.contains(extension)) {
		// PPL (texture)
		auto binary = this->window->readBinaryEntry(path);
		if (!binary) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->hideAllPreviews();
		this->texturePreview->show();
		this->texturePreview->setPPLData(*binary);
	} else if (TexturePreview::EXTENSIONS_TTX.contains(extension)) {
		// TTH/TTZ (VTMB texture)
		auto fsPath = std::filesystem::path{path.toLocal8Bit().constData()};
		QString basePath = (fsPath.parent_path().string() + '/' + fsPath.stem().string()).c_str();
		auto tthBinary = this->window->readBinaryEntry(basePath + ".tth");
		if (!tthBinary) {
			this->showFileLoadErrorPreview();
			return;
		}
		auto ttzBinary = this->window->readBinaryEntry(basePath + ".ttz");
		this->hideAllPreviews();
		this->texturePreview->show();
		this->texturePreview->setTTXData(*tthBinary, ttzBinary ? *ttzBinary : std::vector<std::byte>{});
	} else if (TexturePreview::EXTENSIONS_VTF.contains(extension)) {
		// VTF (texture)
		auto binary = this->window->readBinaryEntry(path);
		if (!binary) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->hideAllPreviews();
		this->texturePreview->show();
		this->texturePreview->setVTFData(*binary);
	} else if (TextPreview::EXTENSIONS.contains(extension)) {
		// Text
		auto text = this->window->readTextEntry(path);
		if (!text) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->hideAllPreviews();
		this->textPreview->show();
		this->textPreview->setText(*text, extension);
	} else {
		this->showInfoPreview({":/icons/warning.png"}, tr("No available preview."));
	}
}

void FileViewer::displayDir(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths, const PackFile& packFile) {
	this->clearContents(false);
	this->navbar->setPath(path);

	this->dirPreview->setPath(path, subfolders, entryPaths, packFile);
	this->hideAllPreviews();
	this->dirPreview->show();
}

void FileViewer::addEntry(const PackFile& packFile, const QString& path) const {
	this->dirPreview->addEntry(packFile, path);
}

void FileViewer::removeFile(const QString& path) const {
	this->dirPreview->removeFile(path);
}

void FileViewer::removeDir(const QString& path) const {
	if (path == this->dirPreview->getCurrentPath()) {
		this->dirPreview->hide();
		return;
	}
	this->dirPreview->removeDir(path);
}

void FileViewer::setSearchQuery(const QString& query) const {
	this->dirPreview->setSearchQuery(query);
}

void FileViewer::setReadOnly(bool readOnly) const {
	this->textPreview->setReadOnly(readOnly);
}

void FileViewer::selectSubItemInDir(const QString& name) const {
	this->window->selectSubItemInDir(name);
}

bool FileViewer::isDirPreviewVisible() const {
	return this->dirPreview->isVisible();
}

const QString& FileViewer::getDirPreviewCurrentPath() const {
	return this->dirPreview->getCurrentPath();
}

void FileViewer::clearContents(bool resetHistory) {
	this->navbar->clearContents(resetHistory);
	this->hideAllPreviews();
	this->emptyPreview->show();
}

void FileViewer::showInfoPreview(const QPixmap& icon, const QString& text) {
	this->hideAllPreviews();
	this->infoPreview->show();
	this->infoPreview->setData(icon, text);
}

void FileViewer::showGenericErrorPreview(const QString& text) {
	this->showInfoPreview({":/icons/error.png"}, text);
}

void FileViewer::showFileLoadErrorPreview() {
	this->showInfoPreview({":/icons/warning.png"}, tr("Failed to read file contents!\nPlease ensure that a game or another application is not using the file."));
}

void FileViewer::hideAllPreviews() {
	for (auto* pluginLoader : this->previewPlugins) {
		qobject_cast<IVPKEditPreviewPlugin_V1_0*>(pluginLoader->instance())->getPreview()->hide();
	}
	this->dirPreview->hide();
	this->infoPreview->hide();
	this->emptyPreview->hide();
	this->textPreview->hide();
	this->texturePreview->hide();
}
