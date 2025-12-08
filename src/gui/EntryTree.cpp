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
#include <QSortFilterProxyModel>
#include <QStyle>
#include <QThread>
#include <QTimer>

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

void EntryTreeNode::sort(Qt::SortOrder order) {
	if (order == Qt::AscendingOrder) {
		std::ranges::sort(this->children_, [](const auto& a, const auto& b) {
			if (a->isDirectory_ != b->isDirectory_) {
				return a->isDirectory_ > b->isDirectory_;
			}
			return a->name_ < b->name_;
		});
	} else {
		std::ranges::sort(this->children_, [](const auto& a, const auto& b) {
			if (a->isDirectory_ != b->isDirectory_) {
				return a->isDirectory_ > b->isDirectory_;
			}
			return a->name_ > b->name_;
		});
	}
	for (const auto& child : this->children_) {
		child->sort(order);
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

	if (!parent.isValid()) {
		if (row == 0) {
			return this->createIndex(row, column, this->root());
		}
		return {};
	}

	const auto* parentNode = EntryTreeModel::getNodeAtIndex(parent);
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

	const auto* childNode = EntryTreeModel::getNodeAtIndex(index);
	if (!childNode || childNode == this->root()) {
		return {};
	}

	const auto* parentNode = childNode->parent();
	if (!parentNode || parentNode == this->root()) {
		return this->createIndex(0, 0, this->root());
	}

	const auto* grandparentNode = parentNode->parent();
	if (!grandparentNode) {
		return {};
	}
	for (int i = 0; i < grandparentNode->children().size(); i++) {
		if (grandparentNode->children()[i].get() == parentNode) {
			return this->createIndex(i, 0, parentNode);
		}
	}
	return {};
}

int EntryTreeModel::rowCount(const QModelIndex& parent) const {
	if (!parent.isValid()) {
		return 1;
	}
	const auto* parentNode = EntryTreeModel::getNodeAtIndex(parent);
	if (!parentNode) {
		return 0;
	}
	return static_cast<int>(parentNode->children().size());
}

int EntryTreeModel::columnCount(const QModelIndex&) const {
	return 1;
}

QVariant EntryTreeModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	const auto* node = EntryTreeModel::getNodeAtIndex(index);
	if (!node) {
		return {};
	}

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
	emit this->layoutAboutToBeChanged();
	this->beginResetModel();
	this->root_ = std::make_unique<EntryTreeNode>(nullptr, "", true);
	this->endResetModel();
	emit this->layoutChanged();
}

void EntryTreeModel::addEntry(const QString& path, bool incremental) {
	QStringList components = path.split('/', Qt::SkipEmptyParts);
	auto* current = this->root();
	if (!current) {
		return;
	}
	for (int i = 0; i < components.size(); i++) {
		auto* child = current->findChild(components[i]);
		if (!child) {
			bool isDir = i < components.size() - 1;
			const auto row = static_cast<int>(current->children().size());
			if (incremental) {
				emit this->layoutAboutToBeChanged();

				QModelIndex parentIndex;
				if (current == this->root()) {
					parentIndex = this->createIndex(0, 0, this->root());
				} else {
					parentIndex = this->getIndexAtNode(current);
				}

				this->beginInsertRows(parentIndex, row, row);
			}
			current->children().push_back(std::make_unique<EntryTreeNode>(current, components[i], isDir));
			if (incremental) {
				this->endInsertRows();
				emit this->layoutChanged();
			}
			child = current->children().back().get();
		}
		current = child;
	}
	if (incremental && current) {
		if (auto* parent = current->parent()) {
			parent->sort(Qt::AscendingOrder);
			emit this->dataChanged(this->getIndexAtNode(parent), this->getIndexAtNode(parent));
		}
	}
}

void EntryTreeModel::removeEntry(const QString& path) {
	auto* node = this->getNodeAtPath(path);
	if (!node || !node->parent()) {
		return;
	}

	emit this->layoutAboutToBeChanged();

	auto* parent = node->parent();
	if (parent) {
		if (const auto it = std::ranges::find_if(parent->children(), [node](const auto& child) {
			return child.get() == node;
		}); it != parent->children().end()) {
			const auto row = static_cast<int>(std::distance(parent->children().begin(), it));

			QModelIndex parentIndex;
			if (parent == this->root()) {
				parentIndex = this->createIndex(0, 0, this->root());
			} else {
				parentIndex = this->getIndexAtNode(parent);
			}

			this->beginRemoveRows(parentIndex, row, row);
			parent->children().erase(it);
			this->endRemoveRows();
		}
	}

	// Remove empty parent directories
	while (parent && parent != this->root() && parent->children().empty()) {
		auto* grandparent = parent->parent();
		if (grandparent) {
			if (const auto it = std::ranges::find_if(grandparent->children(), [parent](const auto& child) {
				return child.get() == parent;
			}); it != grandparent->children().end()) {
				const auto row = static_cast<int>(std::distance(grandparent->children().begin(), it));

				QModelIndex grandparentIndex;
				if (grandparent == this->root()) {
					grandparentIndex = this->createIndex(0, 0, this->root());
				} else {
					grandparentIndex = this->getIndexAtNode(grandparent);
				}

				this->beginRemoveRows(grandparentIndex, row, row);
				grandparent->children().erase(it);
				this->endRemoveRows();
			}
		}
		parent = grandparent;
	}

	emit this->layoutChanged();
}

bool EntryTreeModel::hasEntry(const QString& path) const {
	return !!this->getNodeAtPath(path);
}

EntryTreeNode* EntryTreeModel::getNodeAtPath(const QString& path) const {
	auto* current = this->root_.get();
	if (!current || path.isEmpty() || path == '/') {
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

void EntryTreeModel::sort(int column, Qt::SortOrder order) {
	if (!this->root_ || column != 0) {
		return;
	}
	emit this->layoutAboutToBeChanged();
	this->root_->sort(order);
	emit this->layoutChanged();
}

const EntryTreeNode* EntryTreeModel::root() const {
	return this->root_.get();
}

EntryTreeNode* EntryTreeModel::root() {
	return this->root_.get();
}

QModelIndex EntryTreeModel::getIndexAtNode(const EntryTreeNode* node) const {
	if (!node) {
		return {};
	}
	if (node == this->root()) {
		return this->createIndex(0, 0, node);
	}

	const auto* parent = node->parent();
	if (!parent) {
		return {};
	}

	for (int i = 0; i < parent->children().size(); i++) {
		if (parent->children()[i].get() == node) {
			return this->createIndex(i, 0, node);
		}
	}
	return {};
}

EntryTreeNode* EntryTreeModel::getNodeAtIndex(const QModelIndex& index) {
	if (!index.isValid()) {
		return nullptr;
	}
	return static_cast<EntryTreeNode*>(index.internalPointer());
}

bool EntryTreeFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
	const auto* entryTreeModel = dynamic_cast<EntryTreeModel*>(this->sourceModel());
	return (entryTreeModel && entryTreeModel->root()->parent() == EntryTreeModel::getNodeAtIndex(sourceParent)) || QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
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

	this->proxiedModel = new EntryTreeModel{this};
	this->model = new EntryTreeFilterProxyModel{this};
	this->model->setSourceModel(this->proxiedModel);
	this->model->setDynamicSortFilter(true);
	this->model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	this->model->setFilterKeyColumn(0);
	this->model->setRecursiveFilteringEnabled(true);
	this->QTreeView::setModel(this->model);

	this->setContextMenuPolicy(Qt::CustomContextMenu);
	auto* contextMenuData = new EntryContextMenuData{true, this};

	// Delay this a bit so the widget actually exists
	// todo: fix this with a plugin manager?
	QTimer::singleShot(1, [this, contextMenuData] { this->window->pluginsInitContextMenu(contextMenuData); });

	QObject::connect(this, &QWidget::customContextMenuRequested, this, [this, contextMenuData](const QPoint& pos) {
		contextMenuData->setReadOnly(this->window->isReadOnly());
		if (const auto selectedIndexes = this->selectedIndexes(); selectedIndexes.length() > 1) {
			QStringList paths;
			for (const auto& selectedIndex : selectedIndexes) {
				paths.push_back(this->getIndexPath(selectedIndex));
			}
			// Update plugins
			this->window->pluginsUpdateContextMenu(IVPKEditPreviewPlugin_V1_3::CONTEXT_MENU_TYPE_MIXED, paths);
			// Handle the selected action if stock
			if (const auto* selectedSelectionAction = contextMenuData->contextMenuSelection->exec(this->mapToGlobal(pos)); selectedSelectionAction == contextMenuData->extractSelectedAction) {
				this->extractEntries(paths);
			} else if (selectedSelectionAction == contextMenuData->removeSelectedAction) {
				for (const auto& path : paths) {
					if (!path.isEmpty()) {
						this->removeEntryByPath(path);
					}
				}
			}
		} else if (const auto modelIndex = this->indexAt(pos); !modelIndex.isValid() || EntryTreeModel::getNodeAtIndex(this->model->mapToSource(modelIndex)) == this->proxiedModel->root()) {
			// Update plugins
			this->window->pluginsUpdateContextMenu(IVPKEditPreviewPlugin_V1_3::CONTEXT_MENU_TYPE_ROOT, {""});
			// Handle the selected action if stock
			if (const auto* selectedAllAction = contextMenuData->contextMenuAll->exec(this->mapToGlobal(pos)); selectedAllAction == contextMenuData->extractAllAction) {
				this->window->extractAll();
			} else if (selectedAllAction == contextMenuData->addFileToRootAction) {
				this->window->addFiles(false);
			} else if (selectedAllAction == contextMenuData->addDirToRootAction) {
				this->window->addDir(false);
			}
		} else {
			const auto* node = EntryTreeModel::getNodeAtIndex(this->model->mapToSource(modelIndex));
			const QString path = this->getIndexPath(modelIndex);
			if (node->isDirectory()) {
				// Update plugins
				this->window->pluginsUpdateContextMenu(IVPKEditPreviewPlugin_V1_3::CONTEXT_MENU_TYPE_DIR, {path});
				// Handle the selected action if stock
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
				// Update plugins
				this->window->pluginsUpdateContextMenu(IVPKEditPreviewPlugin_V1_3::CONTEXT_MENU_TYPE_FILE, {path});
				// Handle the selected action if stock
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
		const auto* node = EntryTreeModel::getNodeAtIndex(this->model->mapToSource(index));
		if (node->isDirectory()) {
			return;
		}
		const TempDir tempDir;
		const QString savePath = tempDir.dir().absoluteFilePath(this->proxiedModel->getNodePath(node));
		this->window->extractFile(this->getIndexPath(index), savePath);
		QDesktopServices::openUrl(QUrl::fromLocalFile(savePath));
	});

	QObject::connect(this->proxiedModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, [this](const QModelIndex& index, int start, int end) {
		for (int i = start; i <= end; i++) {
			if (const auto childIndex = this->proxiedModel->index(i, 0, index); childIndex.isValid()) {
				if (const auto* node = EntryTreeModel::getNodeAtIndex(childIndex)) {
					const auto path = this->proxiedModel->getNodePath(node);
					if (node->isDirectory()) {
						this->window->removeDir(path);
					} else {
						this->window->removeFile(path);
					}
				}
			}
		}
	});

	this->clearContents();
}

void EntryTree::loadPackFile(PackFile& packFile, QProgressBar* progressBar, const std::function<void()>& finishCallback) {
	// Set root item name to pack file name
	this->proxiedModel->root()->setName(packFile.getTruncatedFilestem().c_str());

	// Set up progress bar
	progressBar->setRange(0, 0);

	// Don't let the user touch anything
	this->setDisabled(true);

	// Set up thread
	this->workerThread = new QThread(this);
	auto* worker = new LoadPackFileWorker();
	worker->moveToThread(this->workerThread);
	QObject::connect(this->workerThread, &QThread::started, worker, [this, worker, &packFile] {
		worker->run(this, packFile);
	});
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
	return this->proxiedModel->hasEntry(path);
}

void EntryTree::selectEntry(const QString& path) {
	this->selectionModel()->clearSelection();
	QStringList components = path.split('/');
	QModelIndex currentIndex = this->model->index(0, 0, {});
	for (const auto& component : components) {
		const int rowCount = this->model->rowCount(currentIndex);
		for (int i = 0; i < rowCount; ++i) {
			QModelIndex childIndex = this->model->index(i, 0, currentIndex);
			if (const auto* child = EntryTreeModel::getNodeAtIndex(this->model->mapToSource(childIndex)); child->name() == component) {
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
		if (const auto* child = EntryTreeModel::getNodeAtIndex(this->model->mapToSource(childIndex)); child && child->name() == name) {
			this->expand(parentIndex);
			this->setCurrentIndex(childIndex);
			this->expand(childIndex);
			this->scrollTo(childIndex, ScrollHint::PositionAtCenter);
			return;
		}
	}
}

void EntryTree::setSearchQuery(const QString& query) const {
	this->model->setFilterRegularExpression(query);
}

void EntryTree::setAutoExpandDirectoryOnClick(bool enable) {
	this->autoExpandDirectories = enable;
}

void EntryTree::removeEntryByPath(const QString& path) {
	this->setCurrentIndex(QModelIndex{});
	this->proxiedModel->removeEntry(path);
}

void EntryTree::clearContents() const {
	this->proxiedModel->clear();
}

void EntryTree::addEntry(const QString& path, bool incremental) const {
	this->proxiedModel->addEntry(path, incremental);
	if (incremental) {
		this->model->invalidate();
	}
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
		const auto* node = this->proxiedModel->getNodeAtPath(path);
		if (!node) return;
		if (node->isDirectory()) {
			for (const auto& child : node->children()) {
				extractRecurse(this->proxiedModel->getNodePath(child.get()));
			}
		} else {
			const QString itemPath = saveDir + QDir::separator() + (rootDirLen > 0 ? path.sliced(rootDirLen) : path);
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
	const bool allowDirDrag = Options::get<bool>(OPT_ENTRY_TREE_ALLOW_DIR_DRAG);
	const bool allowFileDrag = Options::get<bool>(OPT_ENTRY_TREE_ALLOW_FILE_DRAG);
	if (!allowDirDrag && !allowFileDrag) {
		return;
	}
	if (!allowDirDrag || !allowFileDrag) {
		for (const auto& path : paths) {
			if (
				const bool isDir = this->proxiedModel->getNodeAtPath(path)->isDirectory();
				(!allowDirDrag && isDir) || (!allowFileDrag && !isDir)
			) {
				return;
			}
		}
	}

	const TempDir tempDir;

	auto* drag = new QDrag(this);
	auto* mimeData = new QMimeData();

	this->extractEntries(paths, tempDir.path());

	QList<QUrl> extractedPaths;
	for (const auto& entryName : tempDir.dir().entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
		extractedPaths.push_back(QUrl::fromLocalFile(tempDir.path() + QDir::separator() + entryName));
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

	const auto* node = EntryTreeModel::getNodeAtIndex(this->model->mapToSource(index));
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
				entryPaths << this->proxiedModel->getNodePath(child.get());
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
	return this->proxiedModel->getNodePath(EntryTreeModel::getNodeAtIndex(this->model->mapToSource(index)));
}

void LoadPackFileWorker::run(EntryTree* tree, const PackFile& packFile) {
	emit tree->proxiedModel->layoutAboutToBeChanged();
	tree->proxiedModel->beginResetModel();
	tree->setUpdatesEnabled(false);
	packFile.runForAllEntries([tree](const std::string& path, const Entry&) {
		tree->addEntry(path.c_str(), false);
	});
	tree->proxiedModel->sort(0, Qt::AscendingOrder);
	tree->setUpdatesEnabled(true);
	tree->proxiedModel->endResetModel();
	emit tree->proxiedModel->layoutChanged();
	emit taskFinished();
}
