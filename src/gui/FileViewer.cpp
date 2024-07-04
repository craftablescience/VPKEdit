#include "FileViewer.h"

#include <filesystem>

#include <QHBoxLayout>
#include <QLineEdit>
#include <QTimer>
#include <QToolButton>

#include "previews/AudioPreview.h"
#include "previews/DirPreview.h"
#include "previews/DMXPreview.h"
#include "previews/EmptyPreview.h"
#include "previews/ImagePreview.h"
#include "previews/InfoPreview.h"
#include "previews/MDLPreview.h"
#include "previews/TextPreview.h"
#include "previews/VTFPreview.h"
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

void NavBar::resetButtonIcons() {
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

	auto* audioPreview = newPreview<AudioPreview>(this);
	layout->addWidget(audioPreview);

	auto* dirPreview = newPreview<DirPreview>(this, this->window, this);
	layout->addWidget(dirPreview);

	auto* dmxPreview = newPreview<DMXPreview>(this);
	layout->addWidget(dmxPreview);

	auto* emptyPreview = newPreview<EmptyPreview>(this);
	layout->addWidget(emptyPreview);

	auto* imagePreview = newPreview<ImagePreview>(this);
	layout->addWidget(imagePreview);

	auto* infoPreview = newPreview<InfoPreview>(this);
	layout->addWidget(infoPreview);

	auto* mdlPreview = newPreview<MDLPreview>(this, this->window, this);
	layout->addWidget(mdlPreview);

	auto* textPreview = newPreview<TextPreview>(this);
	layout->addWidget(textPreview);

	auto* vtfPreview = newPreview<VTFPreview>(this);
	layout->addWidget(vtfPreview);

	this->clearContents(true);
}

void FileViewer::requestNavigateBack() {
	this->navbar->navigateBack();
}

void FileViewer::requestNavigateNext() {
	this->navbar->navigateNext();
}

void FileViewer::displayEntry(const QString& path, const PackFile& packFile) {
	// Get extension
	std::filesystem::path helperPath(path.toLower().toLocal8Bit().constData());
	QString extension(helperPath.has_extension() ? helperPath.extension().string().c_str() : helperPath.stem().string().c_str());

	this->clearContents(false);
	this->navbar->setPath(path);

	if (AudioPreview::EXTENSIONS.contains(extension)) {
		// Audio
		auto binary = this->window->readBinaryEntry(path);
		if (!binary) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->showPreview<AudioPreview>();
		this->getPreview<AudioPreview>()->setData(*binary);
	} else if (DMXPreview::EXTENSIONS.contains(extension)) {
		// DMX
		auto binary = this->window->readBinaryEntry(path);
		if (!binary) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->showPreview<DMXPreview>();
		this->getPreview<DMXPreview>()->setData(*binary);
	} else if (ImagePreview::EXTENSIONS.contains(extension)) {
		// Image
		auto binary = this->window->readBinaryEntry(path);
		if (!binary) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->showPreview<ImagePreview>();
		this->getPreview<ImagePreview>()->setData(*binary);
	} else if (MDLPreview::EXTENSIONS.contains(extension)) {
		// MDL (model)
		auto binary = this->window->readBinaryEntry(path);
		if (!binary) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->showPreview<MDLPreview>();
		this->getPreview<MDLPreview>()->setMesh(path, packFile);
	} else if (VTFPreview::EXTENSIONS.contains(extension)) {
		// VTF (texture)
		auto binary = this->window->readBinaryEntry(path);
		if (!binary) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->showPreview<VTFPreview>();
		this->getPreview<VTFPreview>()->setData(*binary);
	} else if (TextPreview::EXTENSIONS.contains(extension)) {
		// Text
		auto text = this->window->readTextEntry(path);
		if (!text) {
			this->showFileLoadErrorPreview();
			return;
		}
		this->showPreview<TextPreview>();
		this->getPreview<TextPreview>()->setText(*text, extension);
	} else {
		this->showInfoPreview({":/icons/warning.png"}, tr("No available preview."));
	}
}

void FileViewer::displayDir(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths, const PackFile& packFile) {
	this->clearContents(false);
	this->navbar->setPath(path);

	this->getPreview<DirPreview>()->setPath(path, subfolders, entryPaths, packFile);
	this->showPreview<DirPreview>();
}

void FileViewer::addEntry(const PackFile& packFile, const QString& path) {
	this->getPreview<DirPreview>()->addEntry(packFile, path);
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

void FileViewer::clearContents(bool resetHistory) {
	this->navbar->clearContents(resetHistory);
	this->showPreview<EmptyPreview>();
}

void FileViewer::showInfoPreview(const QPixmap& icon, const QString& text) {
	for (const auto [index, widget] : this->previews) {
		widget->hide();
	}
	auto* infoPreview = dynamic_cast<InfoPreview*>(this->previews.at(std::type_index(typeid(InfoPreview))));
	infoPreview->show();
	infoPreview->setData(icon, text);
}

void FileViewer::showGenericErrorPreview(const QString& text) {
	this->showInfoPreview({":/icons/error.png"}, text);
}

void FileViewer::showFileLoadErrorPreview() {
	this->showInfoPreview({":/icons/warning.png"}, tr("Failed to read file contents!\nPlease ensure that a game or another application is not using the file."));
}
