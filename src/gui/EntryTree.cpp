#include "EntryTree.h"

#include <algorithm>
#include <filesystem>

#include <QApplication>
#include <QClipboard>
#include <QCollator>
#include <QDesktopServices>
#include <QDrag>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProgressBar>
#include <QStyle>
#include <QThread>

#include "extensions/SingleFile.h"
#include "previews/TextPreview.h"
#include "utility/Options.h"
#include "utility/TempDir.h"
#include "EntryContextMenuData.h"
#include "Window.h"

#if defined(_WIN32)
	#include <windows.h>
#endif

using namespace vpkpp;

namespace {

#ifdef _WIN32
QIcon getIconForExtensionWin(const QString& extension) {
	SHFILEINFO shellFileInfo;
	ZeroMemory(&shellFileInfo, sizeof(SHFILEINFO));
	if (SHGetFileInfo(extension.toStdWString().c_str(), FILE_ATTRIBUTE_NORMAL, &shellFileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES) != ERROR_SUCCESS && shellFileInfo.hIcon) {
		QIcon icon{QPixmap::fromImage(QImage::fromHICON(shellFileInfo.hIcon))};
		DestroyIcon(shellFileInfo.hIcon);
		return icon;
	}
	return {};
}
#endif

const QIcon& getIconForExtension(QString extension) {
	// Convert text extensions to .txt so they don't use the unknown icon
	if (TextPreview::EXTENSIONS.contains(extension)) {
		extension = ".txt";
	}

	// Memoize so we're not constantly searching for an icon that's already been found
	static QMap<QString, QIcon> cachedExtensions;
	if (cachedExtensions.contains(extension)) {
		return cachedExtensions[extension];
	}

#if defined(_WIN32)
	if (auto icon = ::getIconForExtensionWin(extension); !icon.isNull()) {
		cachedExtensions[extension] = icon;
		return cachedExtensions[extension];
	}
#endif
	static QMimeDatabase mimeDatabase;
	QList<QMimeType> mimeTypes = mimeDatabase.mimeTypesForFileName("_" + extension);
	for (const auto& mimeType : mimeTypes) {
		if (auto icon = QIcon::fromTheme(mimeType.iconName()); !icon.isNull()) {
			cachedExtensions[extension] = icon;
			return cachedExtensions[extension];
		}
	}

	// Couldn't find it, use a generic file icon
	cachedExtensions[extension] = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
	return cachedExtensions[extension];
}

QString join(const QStringList& list, const QString& separator) {
	if (list.isEmpty()) {
		return "";
	}
	QString result = list.first();
	for (int i = 1; i < list.size(); ++i) {
		result += separator + list[i];
	}
	return result;
}

} // namespace

class EntryItem : public QTreeWidgetItem {
public:
	using QTreeWidgetItem::QTreeWidgetItem;

	bool operator<(const QTreeWidgetItem& other) const override {
		// Directories should always go above files
		if (!this->childCount() && other.childCount()) {
			return false;
		}
		if (this->childCount() && !other.childCount()) {
			return true;
		}

		// Use QCollator to sort strings with numbers properly
		static QCollator col;
		col.setNumericMode(true);
		return col.compare(this->text(0), other.text(0)) < 0;
	}
};

EntryTree::EntryTree(Window* window_, QWidget* parent)
		: QTreeWidget(parent)
		, window(window_)
		, autoExpandDirectories(false) {
	this->setMinimumWidth(200);
	this->setHeaderHidden(true);
	this->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

	this->workerThread = nullptr;
	this->root = nullptr;

	this->setContextMenuPolicy(Qt::CustomContextMenu);
	auto* contextMenuData = new EntryContextMenuData{true, this};
	QObject::connect(this, &QTreeWidget::customContextMenuRequested, this, [this, contextMenuData](const QPoint& pos) {
		contextMenuData->setReadOnly(this->window->isReadOnly());
		if (this->selectedItems().length() == 1) {
			auto path = this->getItemPath(this->selectedItems()[0]);
			if (path.endsWith(".nuc") || path.endsWith(".ctx")) {
				contextMenuData->setEncryptDecryptVisible(false, true);
			} else if (path.endsWith(".nut") || path.endsWith(".txt")) {
				contextMenuData->setEncryptDecryptVisible(true, false);
			} else {
				contextMenuData->setEncryptDecryptVisible(false, false);
			}
		} else {
			contextMenuData->setEncryptDecryptVisible(false, false);
		}

		if (this->selectedItems().length() > 1) {
			// Show the selection context menu at the requested position
			auto* selectedSelectionAction = contextMenuData->contextMenuSelection->exec(this->mapToGlobal(pos));

			// Handle the selected action
			if (selectedSelectionAction == contextMenuData->extractSelectedAction) {
				QStringList paths;
				for (auto* item : this->selectedItems()) {
					paths.push_back(this->getItemPath(item));
				}
				this->extractEntries(paths);
			} else if (selectedSelectionAction == contextMenuData->removeSelectedAction) {
				for (auto* item : this->selectedItems()) {
					if (item == this->root) {
						continue;
					}
					this->removeEntry(item);
				}
			}
		} else if (auto* selectedItem = this->itemAt(pos)) {
			QString path = this->getItemPath(selectedItem);
			if (path.isEmpty()) {
				// Show the root context menu at the requested position
				auto* selectedAllAction = contextMenuData->contextMenuAll->exec(this->mapToGlobal(pos));

				// Handle the selected action
				if (selectedAllAction == contextMenuData->extractAllAction) {
					this->window->extractAll();
				} else if (selectedAllAction == contextMenuData->addFileToRootAction) {
					this->window->addFiles(false);
				} else if (selectedAllAction == contextMenuData->addDirToRootAction) {
					this->window->addDir(false);
				}
			} else if (selectedItem->childCount() > 0) {
				// Show the directory context menu at the requested position
				auto* selectedDirAction = contextMenuData->contextMenuDir->exec(this->mapToGlobal(pos));

				// Handle the selected action
				if (selectedDirAction == contextMenuData->extractDirAction) {
					this->window->extractDir(path);
				} else if (selectedDirAction == contextMenuData->addFileToDirAction) {
					this->window->addFiles(false, path);
				} else if (selectedDirAction == contextMenuData->addDirToDirAction) {
					this->window->addDir(false, path);
				} else if (selectedDirAction == contextMenuData->renameDirAction) {
					this->window->renameDir(path);
				} else if (selectedDirAction == contextMenuData->copyDirPathAction) {
					QGuiApplication::clipboard()->setText(path);
				} else if (selectedDirAction == contextMenuData->removeDirAction) {
					this->removeEntry(selectedItem);
				}
			} else {
				// Show the file context menu at the requested position
				auto* selectedFileAction = contextMenuData->contextMenuFile->exec(this->mapToGlobal(pos));

				// Handle the selected action
				if (selectedFileAction == contextMenuData->extractFileAction) {
					this->window->extractFile(path);
				} else if (selectedFileAction == contextMenuData->editFileAction) {
					this->window->editFile(path);
				} else if (selectedFileAction == contextMenuData->encryptFileAction) {
					this->window->encryptFile(path);
				} else if (selectedFileAction == contextMenuData->decryptFileAction) {
					this->window->decryptFile(path);
				} else if (selectedFileAction == contextMenuData->copyFilePathAction) {
					QGuiApplication::clipboard()->setText(path);
				} else if (selectedFileAction == contextMenuData->removeFileAction) {
					this->removeEntry(selectedItem);
				}
			}
		}
	});

	QObject::connect(this, &QTreeWidget::currentItemChanged, this, &EntryTree::onCurrentItemChanged);

	QObject::connect(this, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int) {
		if (item->childCount() > 0) {
			return;
		}
		const TempDir tempDir;
		const QString savePath = tempDir.dir().absoluteFilePath(item->text(0));
		this->window->extractFile(this->getItemPath(item), savePath);
		QDesktopServices::openUrl("file://" + savePath);
	});

	this->clearContents();
}

void EntryTree::loadPackFile(PackFile& packFile, QProgressBar* progressBar, const std::function<void()>& finishCallback) {
	// Create root item
	this->root = new EntryItem(this);
	this->root->setText(0, packFile.getTruncatedFilestem().c_str());
	if (!Options::get<bool>(OPT_ENTRY_TREE_HIDE_ICONS)) {
		// Set the icon now even though its set later, that way it will be present while loading
		this->root->setIcon(0, this->style()->standardIcon(QStyle::SP_DirIcon));
	}

	// Set up progress bar
	progressBar->setMinimum(0);
	progressBar->setMaximum(static_cast<int>(packFile.getBakedEntries().size()));
	progressBar->setValue(0);

	// Don't let the user touch anything
	this->setDisabled(true);
	this->root->setDisabled(true);

	// Set up thread
	this->workerThread = new QThread(this);
	auto* worker = new LoadPackFileWorker();
	worker->moveToThread(this->workerThread);
	QObject::connect(this->workerThread, &QThread::started, worker, [this, worker, &packFile] {
		worker->run(this, packFile);
	});
	QObject::connect(worker, &LoadPackFileWorker::progressUpdated, this, [progressBar](int value) {
		progressBar->setValue(value);
	});
	QObject::connect(worker, &LoadPackFileWorker::taskFinished, this, [this, isSingleFile=static_cast<bool>(dynamic_cast<SingleFile*>(&packFile)), finishCallback] {
		// Kill thread
		this->workerThread->quit();
		this->workerThread->wait();
		delete this->workerThread;
		this->workerThread = nullptr;

		// Ok we've loaded let them touch it
		this->setDisabled(false);
		this->root->setDisabled(false);

		// Fire the click manually to show the contents and expand the root
		this->root->setSelected(true);
		this->onCurrentItemChanged(this->root);
		this->root->setExpanded(true);

		// Select the file inside if we're loading that specifically
		if (isSingleFile && this->root->childCount() == 1) {
			this->root->setSelected(false);
			auto* child = this->root->child(0);
			child->setSelected(true);
			this->onCurrentItemChanged(child);
			child->setExpanded(true);
		}

		finishCallback();
	});
	this->workerThread->start();
}

bool EntryTree::hasEntry(const QString& path) const {
	return static_cast<bool>(this->getItemAtPath(path));
}

void EntryTree::selectEntry(const QString& path) {
	for (auto* selected : this->selectedItems()) {
		selected->setSelected(false);
	}
	auto* entry = this->getItemAtPath(path);
	if (!entry) {
		return;
	}

	auto* currentEntry = entry;
	while (currentEntry != this->root) {
		currentEntry->setExpanded(true);
		currentEntry = currentEntry->parent();
	}
	this->root->setExpanded(true);

	entry->setSelected(true);
	this->onCurrentItemChanged(entry);
	entry->setExpanded(true);
	this->scrollToItem(entry, QAbstractItemView::ScrollHint::PositionAtCenter);
}

void EntryTree::selectSubItem(const QString& name) {
	for (auto* selected : this->selectedItems()) {
		selected->setSelected(false);
		selected->setExpanded(true);
		for (int i = 0; i < selected->childCount(); ++i) {
			auto* child = selected->child(i);
			if (child->text(0) == name) {
				child->setSelected(true);
				this->onCurrentItemChanged(child);
				child->setExpanded(true);
				this->scrollToItem(child, QAbstractItemView::ScrollHint::PositionAtCenter);
				return;
			}
		}
	}
}

void EntryTree::setSearchQuery(const QString& query) {
	// Set items that contain a word in the query visible
	const auto words = query.split(' ');
	for (QTreeWidgetItemIterator it(this); *it; ++it) {
		QTreeWidgetItem* item = *it;
		item->setHidden(false);
		for (const auto& word : words) {
			if (item->childCount() == 0 && !item->text(0).contains(word, Qt::CaseInsensitive)) {
				item->setHidden(true);
			}
		}
	}

	// Hide directories that have no children
	int dirsTouched;
	do {
		dirsTouched = 0;
		for (QTreeWidgetItemIterator it(this); *it; ++it) {
			QTreeWidgetItem* item = *it;
			if (item->isHidden() || item->childCount() == 0) {
				continue;
			}
			bool hasChildNotHidden = false;
			for (int i = 0; i < item->childCount(); i++) {
				if (!item->child(i)->isHidden()) {
					hasChildNotHidden = true;
				}
			}
			if (!hasChildNotHidden) {
				dirsTouched++;
				item->setHidden(true);
			}
		}
	} while (dirsTouched != 0);

	this->root->setHidden(false);
}

void EntryTree::setAutoExpandDirectoryOnClick(bool enable) {
	this->autoExpandDirectories = enable;
}

void EntryTree::removeEntryByPath(const QString& path) {
	auto elements = path.split('/');
	auto* currentEntry = this->root;

	while (!elements.isEmpty()) {
		auto* oldEntry = currentEntry;
		for (int i = 0; i < currentEntry->childCount(); i++) {
			if (currentEntry->child(i)->text(0) == elements[0]) {
				elements.pop_front();
				currentEntry = currentEntry->child(i);
				break;
			}
		}
		if (oldEntry == currentEntry) {
			return;
		}
	}

	this->removeEntry(currentEntry);
}

void EntryTree::clearContents() {
	this->root = nullptr;
	this->clear();
}

void EntryTree::addEntry(const QString& path) {
	this->addNestedEntryComponents(path);
	this->sortItems(0, Qt::AscendingOrder);
}

void EntryTree::extractEntries(const QStringList& paths, const QString& destination) {
	// Get destination folder
	QString saveDir = destination;
	if (saveDir.isEmpty()) {
		saveDir = QFileDialog::getExistingDirectory(this, tr("Extract to..."));
	}
	if (saveDir.isEmpty()) {
		return;
	}

	// Strip shared directories until we have a root folder
	QList<QStringList> pathSplits;
	for (const auto& path : paths) {
		pathSplits.push_back(path.split('/'));
	}
	QStringList rootDirList;
	while (true) {
		bool allTheSame = true;
		QString first = pathSplits[0][0];
		for (const auto& path : pathSplits) {
			if (path.length() == 1) {
				allTheSame = false;
				break;
			}
			if (path[0] != first) {
				allTheSame = false;
				break;
			}
		}
		if (!allTheSame) {
			break;
		}
		rootDirList.push_back(std::move(first));
		for (auto& path : pathSplits) {
			path.pop_front();
		}
	}
	// Add one for separator
	qsizetype rootDirLen = ::join(rootDirList, "/").length();

	// Extract
	std::function<void(QTreeWidgetItem*)> extractItemRecurse;
	extractItemRecurse = [&](QTreeWidgetItem* item) {
		if (item->childCount() > 0) {
			for (int i = 0; i < item->childCount(); i++) {
				extractItemRecurse(item->child(i));
			}
		} else {
			const QString itemPath = saveDir + QDir::separator() + this->getItemPath(item).sliced(rootDirLen);
			std::string itemPathStr = itemPath.toLocal8Bit().constData();
			std::filesystem::path itemPathDir(itemPathStr);
			QDir(saveDir).mkpath(itemPathDir.parent_path().string().c_str());

			this->window->extractFile(this->getItemPath(item), itemPath);
		}
	};
	for (const auto& path : paths) {
		extractItemRecurse(this->getItemAtPath(path));
	}
}

void EntryTree::createDrag(const QStringList& paths) {
	TempDir tempDir;

	auto* drag = new QDrag(this);
	auto* mimeData = new QMimeData();

	this->extractEntries(paths, tempDir.path());

	QList<QUrl> extractedPaths;
	QStringList extractedRawPaths = tempDir.dir().entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
	for (const auto& rawPath : extractedRawPaths) {
		extractedPaths.push_back(QUrl::fromLocalFile(tempDir.path() + QDir::separator() + rawPath));
	}

	// Set up drag
	this->window->setDropEnabled(false);

	mimeData->setUrls(extractedPaths);
	drag->setMimeData(mimeData);
	drag->exec(Qt::MoveAction);

	this->window->setDropEnabled(true);
}

void EntryTree::onCurrentItemChanged(QTreeWidgetItem* item) const {
	if (!item) {
		return;
	}

	if (this->autoExpandDirectories) {
		item->setExpanded(!item->isExpanded());
	}

	auto path = this->getItemPath(item);
	if (item->childCount() == 0) {
		this->window->selectEntryInFileViewer(path);
	} else {
		QList<QString> subfolders;
		QList<QString> entryPaths;
		for (int i = 0; i < item->childCount(); i++) {
			auto* child = item->child(i);
			if (child->childCount() == 0) {
				entryPaths << this->getItemPath(child);
			} else {
				subfolders << child->text(0);
			}
		}
		this->window->selectDirInFileViewer(path, subfolders, entryPaths);
	}
}

void EntryTree::keyPressEvent(QKeyEvent* event) {
	if (event->keyCombination().key() == Qt::Key_Delete) {
		event->accept();
		for (auto* item : this->selectedItems()) {
			if (event->keyCombination().keyboardModifiers() != Qt::SHIFT) {
				auto reply = QMessageBox::question(this, tr("Delete Entry"), tr("Are you sure you want to delete \"%1\"?\n(Hold Shift to skip this popup.)").arg(this->getItemPath(item)), QMessageBox::Ok | QMessageBox::Cancel);
				if (reply == QMessageBox::Cancel) {
					continue;
				}
			}
			this->removeEntry(item);
		}
	}
	QTreeWidget::keyPressEvent(event);
}

void EntryTree::mousePressEvent(QMouseEvent* event) {
	this->dragStartPos = event->pos();
	this->dragSelectedItems = this->selectedItems();

	QTreeWidget::mousePressEvent(event);
}

void EntryTree::mouseMoveEvent(QMouseEvent* event) {
	if (!(event->buttons() & Qt::LeftButton)) {
		return QTreeWidget::mouseMoveEvent(event);
	}
	if ((event->pos() - this->dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
		return QTreeWidget::mouseMoveEvent(event);
	}
	if (this->dragSelectedItems.isEmpty()) {
		return QTreeWidget::mouseMoveEvent(event);
	}
	event->accept();

	QStringList paths;
	for (auto* item : this->dragSelectedItems) {
		paths.push_back(this->getItemPath(item));
	}
	this->createDrag(paths);
}

QString EntryTree::getItemPath(QTreeWidgetItem* item) const {
	// Traverse up the item hierarchy until reaching the root item
	QString path;
	for ( ; item && item != this->root; item = item->parent()) {
		if (!path.isEmpty()) {
			path.prepend('/');
		}
		path.prepend(item->text(0));
	}
	return path;
}

QTreeWidgetItem* EntryTree::getItemAtPath(const QString& path) const {
	if (path.isEmpty()) {
		return this->root;
	}

	// Traverse down the item hierarchy until reaching the lowest item
	auto pieces = path.split('/');
	auto* currentEntry = this->root;
	while (currentEntry && !pieces.isEmpty()) {
		bool found = false;
		for (int i = 0; i < currentEntry->childCount() && !found; i++) {
			if (currentEntry->child(i)->text(0) == pieces[0]) {
				currentEntry = currentEntry->child(i);
				pieces.pop_front();
				found = true;
			}
		}
		if (!found) {
			currentEntry = nullptr;
		}
	}
	if (!pieces.isEmpty()) {
		return nullptr;
	}
	return currentEntry;
}

void EntryTree::addNestedEntryComponents(const QString& path) const {
	QStringList components = path.split('/', Qt::SkipEmptyParts);
	QTreeWidgetItem* currentItem = nullptr;

	for (int i = 0; i < components.size(); i++) {
		QTreeWidgetItem* newItem = nullptr;

		// Find the child item with the current component text under the current parent item
		int childCount = currentItem ? currentItem->childCount() : this->root->childCount();
		for (int j = 0; j < childCount; j++) {
			QTreeWidgetItem* childItem = currentItem ? currentItem->child(j) : this->root->child(j);
			if (childItem->text(0) == components[i]) {
				newItem = childItem;
				break;
			}
		}

		// If the child item doesn't exist, create a new one
		if (!newItem) {
			if (currentItem) {
				newItem = new EntryItem(currentItem, {components[i]});
			} else {
				newItem = new EntryItem(this->root, {components[i]});
			}

			if (!Options::get<bool>(OPT_ENTRY_TREE_HIDE_ICONS)) {
				if (i != components.size() - 1) { // is a directory
					newItem->setIcon(0, this->style()->standardIcon(QStyle::SP_DirIcon));
				} else {
					newItem->setIcon(0, ::getIconForExtension("." + QFileInfo(components[i]).suffix()));
				}
			}
		}

		currentItem = newItem;
	}
}

void EntryTree::removeEntry(QTreeWidgetItem* item) {
	auto* parent = item->parent();
	this->removeEntryRecurse(item);

	// Remove dead directories
	while (parent && parent != this->root && parent->childCount() == 0) {
		this->window->removeDir(this->getItemPath(parent));
		auto* temp = parent->parent();
		delete parent;
		parent = temp;
	}
}

// NOLINTNEXTLINE(*-no-recursion)
void EntryTree::removeEntryRecurse(QTreeWidgetItem* item) {
	if (item->childCount() == 0) {
		this->window->removeFile(this->getItemPath(item));
	}
	while (item->childCount() > 0) {
		this->removeEntryRecurse(item->child(0));
	}
	delete item;
}

void LoadPackFileWorker::run(EntryTree* tree, const PackFile& packFile) {
	int progress = 0;
	packFile.runForAllEntries([this, tree, &progress](const std::string& path, const Entry& entry) {
		emit this->progressUpdated(++progress);
		tree->addNestedEntryComponents(QString(path.c_str()));
	});
	tree->sortItems(0, Qt::AscendingOrder);
	emit taskFinished();
}
