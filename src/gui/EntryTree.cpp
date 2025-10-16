// ReSharper disable CppRedundantQualifier

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
	for (const auto& mimeType : mimeDatabase.mimeTypesForFileName("_" + extension)) {
		if (auto icon = QIcon::fromTheme(mimeType.iconName()); !icon.isNull()) {
			cachedExtensions[extension] = icon;
			return cachedExtensions[extension];
		}
	}

	// Couldn't find it, use a generic file icon
	cachedExtensions[extension] = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
	return cachedExtensions[extension];
}

} // namespace

EntryTreeNode::EntryTreeNode(EntryTreeNode* parent, QString name, bool isDirectory)
		: parent_(parent)
		, name_(std::move(name))
		, isDirectory_(isDirectory) {}

EntryTreeNode* EntryTreeNode::findChild(const QString& name) const {
	for (const auto& child : this->children_) {
		if (child->name_ == name) {
			return child.get();
		}
	}
	return nullptr;
}

void EntryTreeNode::sort() {
	std::ranges::sort(this->children_, [](const auto& a, const auto& b) {
		if (a->isDirectory_ != b->isDirectory_) {
			return a->isDirectory_ > b->isDirectory_;
		}
		return a->name_ < b->name_;
	});
	for (const auto& child : this->children_) {
		child->sort();
	}
}

const EntryTreeNode* EntryTreeNode::parent() const {
	return this->parent_;
}

EntryTreeNode* EntryTreeNode::parent() {
	return this->parent_;
}

const QString& EntryTreeNode::name() const {
	return this->name_;
}

void EntryTreeNode::setName(QString name) {
	this->name_ = std::move(name);
}

const std::vector<std::unique_ptr<EntryTreeNode>>& EntryTreeNode::children() const {
	return this->children_;
}

std::vector<std::unique_ptr<EntryTreeNode>> & EntryTreeNode::children() {
	return this->children_;
}

bool EntryTreeNode::isDirectory() const {
	return this->isDirectory_;
}

EntryTreeModel::EntryTreeModel(QObject* parent)
		: QAbstractItemModel(parent)
		, root_(std::make_unique<EntryTreeNode>(nullptr, "", true)) {}

QModelIndex EntryTreeModel::index(int row, int column, const QModelIndex& parent) const {
	if (!this->hasIndex(row, column, parent)) {
		return {};
	}

	if (this->root()->name().isEmpty()) {
		const auto* parentNode = parent.isValid() ? this->getNodeAtIndex(parent) : this->root();
		if (row < 0 || row >= static_cast<int>(parentNode->children().size())) {
			return {};
		}
		return this->createIndex(row, column, parentNode->children()[row].get());
	}

	if (!parent.isValid()) {
		if (row == 0) {
			return this->createIndex(row, column, this->root());
		}
		return {};
	}

	const auto* parentNode = this->getNodeAtIndex(parent);
	if (parentNode == this->root()) {
		if (row < 0 || row >= static_cast<int>(this->root()->children().size())) {
			return {};
		}
		return this->createIndex(row, column, this->root()->children()[row].get());
	}

	if (row < 0 || row >= static_cast<int>(parentNode->children().size())) {
		return {};
	}
	return this->createIndex(row, column, parentNode->children()[row].get());
}

QModelIndex EntryTreeModel::parent(const QModelIndex& index) const {
	if (!index.isValid()) {
		return {};
	}

	const auto* childNode = this->getNodeAtIndex(index);
	if (childNode == this->root()) {
		return {};
	}

	const auto* parentNode = childNode->parent();
	if (parentNode == this->root()) {
		return this->createIndex(0, 0, this->root());
	}

	const auto* grandparentNode = parentNode->parent();
	for (int i = 0; i < grandparentNode->children().size(); i++) {
		if (grandparentNode->children()[i].get() == parentNode) {
			return this->createIndex(i, 0, parentNode);
		}
	}
	return {};
}

int EntryTreeModel::rowCount(const QModelIndex& parent) const {
	return static_cast<int>((parent.isValid() ? this->getNodeAtIndex(parent) : this->root())->children().size());
}

int EntryTreeModel::columnCount(const QModelIndex&) const {
	return 1;
}

QVariant EntryTreeModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	const auto* node = this->getNodeAtIndex(index);
	if (role == Qt::DisplayRole) {
		return node->name();
	}
	if (role == Qt::DecorationRole) {
		if (!Options::get<bool>(OPT_ENTRY_TREE_HIDE_ICONS)) {
			if (node->isDirectory()) {
				return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
			}
			return ::getIconForExtension("." + QFileInfo{node->name()}.suffix());
		}
	}
	return {};
}

Qt::ItemFlags EntryTreeModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void EntryTreeModel::clear() {
	this->beginResetModel();
	this->root_ = std::make_unique<EntryTreeNode>(nullptr, "", true);
	this->endResetModel();
}

void EntryTreeModel::addEntry(const QString& path, bool sort) {
	QStringList components = path.split('/', Qt::SkipEmptyParts);
	auto* current = this->root_.get();
	for (int i = 0; i < components.size(); i++) {
		auto* child = current->findChild(components[i]);
		if (!child) {
			bool isDir = i < components.size() - 1;
			current->children().push_back(std::make_unique<EntryTreeNode>(current, components[i], isDir));
			child = current->children().back().get();
		}
		current = child;
	}
	if (sort && current) {
		current->parent()->sort();
	}
	this->layoutChanged();
}

void EntryTreeModel::removeEntry(const QString& path) {
	auto* node = this->getNodeAtPath(path);
	if (!node || !node->parent()) {
		return;
	}

	if (const auto it = std::ranges::find_if(node->parent()->children(), [node](const auto& child) {
		return child.get() == node;
	}); it != node->parent()->children().end()) {
		node->parent()->children().erase(it);
	}

	// Remove empty parent directories
	auto* parent = node->parent();
	while (parent && parent != this->root() && parent->children().empty()) {
		auto* grandparent = parent->parent();
		if (grandparent) {
			if (const auto it = std::ranges::find_if(grandparent->children(), [parent](const auto& child) {
				return child.get() == parent;
			}); it != grandparent->children().end()) {
				grandparent->children().erase(it);
			}
		}
		parent = grandparent;
	}
	this->layoutChanged();
}

bool EntryTreeModel::hasEntry(const QString& path) const {
	return !!this->getNodeAtPath(path);
}

EntryTreeNode* EntryTreeModel::getNodeAtPath(const QString& path) const {
	auto* current = this->root_.get();
	if (path.isEmpty() || path == '/') {
		return current;
	}
	for (const auto& component : path.split('/')) {
		current = current->findChild(component);
		if (!current) {
			return nullptr;
		}
	}
	return current;
}

QString EntryTreeModel::getNodePath(const EntryTreeNode* node) const {
	QString path;
	while (node && node != this->root()) {
		if (!path.isEmpty()) {
			path.prepend('/');
		}
		path.prepend(node->name());
		node = node->parent();
	}
	return path;
}

void EntryTreeModel::sort(int, Qt::SortOrder) {
	this->sort();
}

void EntryTreeModel::sort() {
	this->root_->sort();
	this->layoutChanged();
}

const EntryTreeNode* EntryTreeModel::root() const {
	return this->root_.get();
}

EntryTreeNode* EntryTreeModel::root() {
	return this->root_.get();
}

EntryTreeNode* EntryTreeModel::getNodeAtIndex(const QModelIndex& index) const {
	return static_cast<EntryTreeNode*>(index.internalPointer());
}

EntryTree::EntryTree(Window* window_, QWidget* parent)
		: QTreeView(parent)
		, window(window_)
		, workerThread(nullptr)
		, autoExpandDirectories(false) {
	this->setMinimumWidth(200);
	this->setHeaderHidden(true);
	this->setSelectionMode(SelectionMode::ExtendedSelection);
	this->setUniformRowHeights(true);

	this->model = new EntryTreeModel{this};
	this->QTreeView::setModel(this->model);

	this->setContextMenuPolicy(Qt::CustomContextMenu);
	auto* contextMenuData = new EntryContextMenuData{true, this};

	QObject::connect(this, &QWidget::customContextMenuRequested, this, [this, contextMenuData](const QPoint& pos) {
		contextMenuData->setReadOnly(this->window->isReadOnly());
		if (const auto selectedIndexes = this->selectedIndexes(); selectedIndexes.length() > 1) {
			// Handle the selected action
			if (const auto* selectedSelectionAction = contextMenuData->contextMenuSelection->exec(this->mapToGlobal(pos)); selectedSelectionAction == contextMenuData->extractSelectedAction) {
				QStringList paths;
				for (const auto& selectedIndex : selectedIndexes) {
					paths.push_back(this->getIndexPath(selectedIndex));
				}
				this->extractEntries(paths);
			} else if (selectedSelectionAction == contextMenuData->removeSelectedAction) {
				for (const auto& selectedIndex : selectedIndexes) {
					if (QString path = this->getIndexPath(selectedIndex); !path.isEmpty()) {
						this->removeEntryByPath(path);
					}
				}
			}
		} else if (const auto modelIndex = this->indexAt(pos); !modelIndex.isValid() || this->model->getNodeAtIndex(modelIndex) == this->model->root()) {
			if (const auto* selectedAllAction = contextMenuData->contextMenuAll->exec(this->mapToGlobal(pos)); selectedAllAction == contextMenuData->extractAllAction) {
				this->window->extractAll();
			} else if (selectedAllAction == contextMenuData->addFileToRootAction) {
				this->window->addFiles(false);
			} else if (selectedAllAction == contextMenuData->addDirToRootAction) {
				this->window->addDir(false);
			}
		} else {
			const auto* node = this->model->getNodeAtIndex(modelIndex);
			const QString path = this->getIndexPath(modelIndex);
			if (node->isDirectory()) {
				if (const auto* selectedDirAction = contextMenuData->contextMenuDir->exec(this->mapToGlobal(pos)); selectedDirAction == contextMenuData->extractDirAction) {
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
					this->removeEntryByPath(path);
				}
			} else {
				if (const auto* selectedFileAction = contextMenuData->contextMenuFile->exec(this->mapToGlobal(pos)); selectedFileAction == contextMenuData->extractFileAction) {
					this->window->extractFile(path);
				} else if (selectedFileAction == contextMenuData->editFileAction) {
					this->window->editFile(path);
				} else if (selectedFileAction == contextMenuData->copyFilePathAction) {
					QGuiApplication::clipboard()->setText(path);
				} else if (selectedFileAction == contextMenuData->removeFileAction) {
					this->removeEntryByPath(path);
				}
			}
		}
	});

	QObject::connect(this->selectionModel(), &QItemSelectionModel::currentChanged, this, &EntryTree::onCurrentIndexChanged);

	QObject::connect(this, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
		const auto* node = this->model->getNodeAtIndex(index);
		if (node->isDirectory()) {
			return;
		}
		const TempDir tempDir;
		const QString savePath = tempDir.dir().absoluteFilePath(this->model->getNodePath(node));
		this->window->extractFile(this->getIndexPath(index), savePath);
		QDesktopServices::openUrl("file://" + savePath);
	});

	this->clearContents();
}

void EntryTree::loadPackFile(PackFile& packFile, QProgressBar* progressBar, const std::function<void()>& finishCallback) {
	// Set root item name to pakfile name
	this->model->root()->setName(packFile.getTruncatedFilestem().c_str());

	// Set up progress bar
	progressBar->setMinimum(0);
	progressBar->setMaximum(static_cast<int>(packFile.getBakedEntries().size()));
	progressBar->setValue(0);

	// Don't let the user touch anything
	this->setDisabled(true);

	// Set up thread
	this->workerThread = new QThread(this);
	auto* worker = new LoadPackFileWorker();
	worker->moveToThread(this->workerThread);
	QObject::connect(this->workerThread, &QThread::started, worker, [this, worker, &packFile] {
		worker->run(this, packFile);
	});
	QObject::connect(worker, &LoadPackFileWorker::progressUpdated, progressBar, &QProgressBar::setValue);
	QObject::connect(worker, &LoadPackFileWorker::taskFinished, this, [this, isSingleFile=static_cast<bool>(dynamic_cast<SingleFile*>(&packFile)), finishCallback] {
		// Kill thread
		this->workerThread->quit();
		this->workerThread->wait();
		delete this->workerThread;
		this->workerThread = nullptr;

		// Ok we've loaded let them touch it
		this->setDisabled(false);

		// Fire the click manually to show the contents and expand the root
		const auto rootIndex = this->model->index(0, 0, {});
		this->setCurrentIndex(rootIndex);
		this->expand(rootIndex);

		// Select the file inside if we're loading that specifically
		if (isSingleFile && this->model->rowCount(rootIndex) == 1) {
			const auto fileIndex = this->model->index(0, 0, rootIndex);
			this->setCurrentIndex(fileIndex);
		}

		finishCallback();
	});
	this->workerThread->start();
}

bool EntryTree::hasEntry(const QString& path) const {
	return this->model->hasEntry(path);
}

void EntryTree::selectEntry(const QString& path) {
	this->selectionModel()->clearSelection();
	QStringList components = path.split('/');
	QModelIndex currentIndex = this->model->index(0, 0, {});
	for (const auto& component : components) {
		const int rowCount = this->model->rowCount(currentIndex);
		for (int i = 0; i < rowCount; ++i) {
			QModelIndex childIndex = this->model->index(i, 0, currentIndex);
			if (const auto* child = this->model->getNodeAtIndex(childIndex); child->name() == component) {
				currentIndex = childIndex;
				this->expand(currentIndex);
				break;
			}
		}
	}

	this->setCurrentIndex(currentIndex);
	this->scrollTo(currentIndex, ScrollHint::PositionAtCenter);
}

void EntryTree::selectSubItem(const QString& name) {
	const auto selectedIndexes = this->selectedIndexes();
	if (selectedIndexes.isEmpty()) {
		return;
	}

	const QModelIndex parentIndex = selectedIndexes[0];
	const int rowCount = this->model->rowCount(parentIndex);
	for (int i = 0; i < rowCount; ++i) {
		QModelIndex childIndex = this->model->index(i, 0, parentIndex);
		if (const auto* child = this->model->getNodeAtIndex(childIndex); child && child->name() == name) {
			this->expand(parentIndex);
			this->setCurrentIndex(childIndex);
			this->expand(childIndex);
			this->scrollTo(childIndex, ScrollHint::PositionAtCenter);
			return;
		}
	}
}

void EntryTree::setSearchQuery(const QString& query) const {
	// todo: reimplement search
}

void EntryTree::setAutoExpandDirectoryOnClick(bool enable) {
	this->autoExpandDirectories = enable;
}

void EntryTree::removeEntryByPath(const QString& path) const {
	this->model->removeEntry(path);
}

void EntryTree::clearContents() const {
	this->model->clear();
}

void EntryTree::addEntry(const QString& path, bool sort) const {
	this->model->addEntry(path, sort);
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

	qsizetype rootDirLen = 0;
	for (const auto& rootDir : rootDirList) {
		// Add one for separator
		rootDirLen += rootDir.length() + 1;
	}
	rootDirLen--;

	std::function<void(const QString&)> extractRecurse = [&](const QString& path) {
		const auto* node = this->model->getNodeAtPath(path);
		if (!node) return;
		if (node->isDirectory()) {
			for (const auto& child : node->children()) {
				extractRecurse(model->getNodePath(child.get()));
			}
		} else {
			const QString itemPath = saveDir + QDir::separator() + path.sliced(rootDirLen);
			const std::string itemPathStr = itemPath.toLocal8Bit().constData();
			const std::filesystem::path itemPathDir(itemPathStr);
			std::ignore = QDir(saveDir).mkpath(itemPathDir.parent_path().string().c_str());
			this->window->extractFile(path, itemPath);
		}
	};

	for (const auto& path : paths) {
		extractRecurse(path);
	}
}

void EntryTree::createDrag(const QStringList& paths) {
	const TempDir tempDir;

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

void EntryTree::onCurrentIndexChanged(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	if (this->autoExpandDirectories) {
		this->setExpanded(index, !this->isExpanded(index));
	}

	const auto* node = this->model->getNodeAtIndex(index);
	const QString path = this->getIndexPath(index);
	if (!node->isDirectory()) {
		this->window->selectEntryInFileViewer(path);
	} else {
		QList<QString> subfolders;
		QList<QString> entryPaths;
		for (const auto& child : node->children()) {
			if (child->isDirectory()) {
				subfolders << child->name();
			} else {
				entryPaths << this->model->getNodePath(child.get());
			}
		}
		this->window->selectDirInFileViewer(path, subfolders, entryPaths);
	}
}

void EntryTree::keyPressEvent(QKeyEvent* event) {
	if (event->keyCombination().key() == Qt::Key_Delete) {
		event->accept();
		for (const auto& selectedIndex : this->selectedIndexes()) {
			if (QString path = this->getIndexPath(selectedIndex); !path.isEmpty()) {
				if (event->keyCombination().keyboardModifiers() != Qt::SHIFT) {
					if (const auto reply = QMessageBox::question(this, tr("Delete Entry"), tr("Are you sure you want to delete \"%1\"?\n(Hold Shift to skip this popup.)").arg(path), QMessageBox::Ok | QMessageBox::Cancel); reply == QMessageBox::Cancel) {
						continue;
					}
				}
				this->removeEntryByPath(path);
			}
		}
	}
	QTreeView::keyPressEvent(event);
}

void EntryTree::mousePressEvent(QMouseEvent* event) {
	this->dragStartPos = event->pos();
	this->dragSelectedIndexes = this->selectedIndexes();

	QTreeView::mousePressEvent(event);
}

void EntryTree::mouseMoveEvent(QMouseEvent* event) {
	if (!(event->buttons() & Qt::LeftButton)) {
		return QTreeView::mouseMoveEvent(event);
	}
	if ((event->pos() - this->dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
		return QTreeView::mouseMoveEvent(event);
	}
	if (this->dragSelectedIndexes.isEmpty()) {
		return QTreeView::mouseMoveEvent(event);
	}
	event->accept();

	QStringList paths;
	for (const auto& selectedIndex : this->dragSelectedIndexes) {
		paths.push_back(this->getIndexPath(selectedIndex));
	}
	this->createDrag(paths);
}

QString EntryTree::getIndexPath(const QModelIndex& index) const {
	if (!index.isValid()) {
		return "";
	}
	return this->model->getNodePath(this->model->getNodeAtIndex(index));
}

void LoadPackFileWorker::run(EntryTree* tree, const PackFile& packFile) {
	int progress = 0;
	packFile.runForAllEntries([this, tree, &progress](const std::string& path, const Entry&) {
		emit this->progressUpdated(++progress);
		tree->addEntry(path.c_str(), false);
	});
	tree->model->sort();
	emit taskFinished();
}
