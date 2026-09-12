#include "VEntryTreeView.hpp"

#include <bsppp/PakLump.h>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QThread>
#include <vtfpp/VTF.h>

#include "Config.hpp"

#include "plugins/thumbnails/IVPKEditThumbnailPlugin.hpp"
#include "utility/VPluginLoader.hpp"
#include "utility/VSettings.hpp"

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

constexpr auto STYLE_DIR_ICON = QStyle::SP_DirIcon;

[[nodiscard]] QIcon& cachedDirIcon() {
	static QIcon DIR_ICON = QApplication::style()->standardIcon(STYLE_DIR_ICON);
	return DIR_ICON;
}

#ifdef Q_OS_WIN
[[nodiscard]] QIcon getIconForExtensionWin32(const QString& extension) {
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

[[nodiscard]] const QIcon& cachedFileIcon(const QString& filename) {
	const auto extension = '.' + QFileInfo{filename}.suffix();

	// Memoize so we're not constantly searching for an icon that's already been found
	static QMap<QString, QIcon> cachedExtensions;
	if (cachedExtensions.contains(extension)) {
		return cachedExtensions[extension];
	}

#ifdef Q_OS_WIN
	if (auto icon = ::getIconForExtensionWin32(filename); !icon.isNull()) {
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

int VEntryTreeItem::childCount() const {
	return static_cast<int>(this->children.size());
}

QString VEntryTreeItem::filename() const {
	if (this->data.index() == 0) [[likely]] {
		return std::get<QString>(this->data);
	}
	return std::get<std::shared_ptr<vpkpp::PackFile>>(this->data)->getFilename().c_str();
}

QString VEntryTreeItem::path() const {
	QString out = this->filename();
	const auto* item = this->parent;
	while (item->parent) {
		out = item->filename() + '/' + out;
		item = item->parent;
	}
	return out;
}

bool VEntryTreeItem::isPackFile() const {
	return this->data.index() == 1;
}

bool VEntryTreeItem::isDirectory() const {
	return this->childCount() > 0;
}

int VEntryTreeItem::row() const {
	if (!this->parent) {
		return 0;
	}
	if (
		const auto it = std::ranges::find_if(std::as_const(this->parent->children), [this](const std::unique_ptr<VEntryTreeItem>& item) { return item.get() == this; });
		it != this->parent->children.cend()
	) {
		return static_cast<int>(std::distance(this->parent->children.cbegin(), it));
	}
	return -1;
}

VEntryTreeItem* VEntryTreeItem::addChild(Data data_) {
	this->children.emplace_back(new VEntryTreeItem{std::move(data_), this});
	return this->children.back().get();
}

void VEntryTreeItem::sort(Qt::SortOrder order) {
	std::ranges::sort(this->children, order == Qt::AscendingOrder ? [](const std::unique_ptr<VEntryTreeItem>& a, const std::unique_ptr<VEntryTreeItem>& b) {
		if (a->isDirectory() != b->isDirectory()) return a->isDirectory() > b->isDirectory();
		return a->filename() < b->filename();
	} : [](const std::unique_ptr<VEntryTreeItem>& a, const std::unique_ptr<VEntryTreeItem>& b) {
		if (a->isDirectory() != b->isDirectory()) return a->isDirectory() > b->isDirectory();
		return a->filename() > b->filename();
	});
	for (const auto& child : this->children) {
		child->sort(order);
	}
}

QVariant VEntryTreeModel::data(const QModelIndex& index, int role) const {
	auto* item = static_cast<const VEntryTreeItem*>(index.internalPointer());
	if (index.isValid()) {
		if (role == Qt::DisplayRole) {
			return item->filename();
		}
		if (role == Qt::DecorationRole) {
			if (VSettings::get<bool>(VSettings::BOOL_ENTRY_TREE_HIDE_ICONS)) {
				return {};
			}
			if (item->childCount() > 0) {
				return ::cachedDirIcon();
			}
			return ::cachedFileIcon(item->filename());
		}
	}
	return {};
}

Qt::ItemFlags VEntryTreeModel::flags(const QModelIndex& index) const {
	return index.isValid() ? QAbstractItemModel::flags(index) : Qt::ItemFlags{Qt::NoItemFlags};
}

QVariant VEntryTreeModel::headerData(int, Qt::Orientation, int) const { // NOLINT(*-default-arguments)
	return {};
}

QModelIndex VEntryTreeModel::index(int row, int column, const QModelIndex& parent) const { // NOLINT(*-default-arguments)
	if (!this->hasIndex(row, column, parent)) {
		return {};
	}
	const auto* parentItem = parent.isValid()
		? static_cast<VEntryTreeItem*>(parent.internalPointer())
		: this->rootItem.get();
	if (const auto* childItem = parentItem->child(row)) {
		return this->createIndex(row, column, childItem);
	}
	return {};
}

QModelIndex VEntryTreeModel::parent(const QModelIndex& index) const {
	if (!index.isValid()) {
		return {};
	}
	const auto* parentItem = static_cast<VEntryTreeItem*>(index.internalPointer())->parentItem();
	return parentItem != this->rootItem.get() ? this->createIndex(parentItem->row(), 0, parentItem) : QModelIndex{};
}

int VEntryTreeModel::rowCount(const QModelIndex& parent) const { // NOLINT(*-default-arguments)
	if (parent.column() > 0) {
		return 0;
	}
	return (
		parent.isValid()
			? static_cast<const VEntryTreeItem*>(parent.internalPointer())
			: this->rootItem.get()
	)->childCount();
}

int VEntryTreeModel::columnCount(const QModelIndex&) const { // NOLINT(*-default-arguments)
	return 1;
}

void VEntryTreeModel::addPackFile(const QString& path) {
	// todo: search for existing path, bail if found

	auto* packFile = vpkpp::PackFile::open(path.toUtf8().constData()).release();
	if (!packFile) {
		// todo: show an error message
		return;
	}

	this->beginResetModel();

	auto* currentRootItem = this->rootItem->addChild(std::shared_ptr<vpkpp::PackFile>{packFile});

	packFile->runForAllEntries([currentRootItem](const std::string& entryPath, const vpkpp::Entry&) {
		// todo: this sucks
		QStringList parts = QString{entryPath.c_str()}.split('/', Qt::SkipEmptyParts);

		auto* currentParent = currentRootItem;
		for (const auto& part : parts) {
			VEntryTreeItem* foundChild = nullptr;
			for (int i = 0; i < currentParent->childCount(); ++i) {
				if (auto* child = currentParent->child(i); child->filename() == part) {
					foundChild = child;
					break;
				}
			}
			if (!foundChild) {
				foundChild = currentParent->addChild(part);
			}
			currentParent = foundChild;
		}
	});

	currentRootItem->sort();

	this->endResetModel();
}

VEntryTreeView::VEntryTreeView(QWidget* parent) : QTreeView{parent} {
	this->QTreeView::setModel(new VEntryTreeModel{this});
	this->setHeaderHidden(true);
	this->setAlternatingRowColors(true);

	this->thumbnailProvider_ = new VEntryTreeThumbnailProvider{this};

	connect(this->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current, const QModelIndex& /*previous*/) {
		if (!current.isValid()) {
			emit this->fileSelected("", {});
			return;
		}

		auto* item = static_cast<VEntryTreeItem*>(current.internalPointer());
		if (item->isPackFile()) {
			// todo: provide data
			return;
		}

		// The full path in the tree needs to be truncated to load the file from the parent pack file
		QString internalItemPath = item->filename();
		auto* itemPackFileParent = item->parentItem();
		while (itemPackFileParent && !itemPackFileParent->isPackFile()) {
			internalItemPath = itemPackFileParent->filename() + '/' + internalItemPath;
			itemPackFileParent = itemPackFileParent->parentItem();
		}
		if (!itemPackFileParent || !itemPackFileParent->isPackFile()) {
			// todo: error (should never see this?)
			return;
		}

		const auto packFile = item->packFile().lock();
		if (!packFile) {
			// todo: error (failed to lock pack file)
			return;
		}

		if (item->isDirectory()) {
			QStringList subdirs;
			QMap<QString, std::optional<vpkpp::Entry>> subfiles;
			QStringList filepaths;
			for (int i = 0; i < item->childCount(); i++) {
				if (const auto* itemChild = item->child(i); itemChild->isDirectory()) {
					subdirs.append(itemChild->filename());
				} else {
					filepaths.append(internalItemPath + '/' + itemChild->filename());
					subfiles[itemChild->filename()] = packFile->findEntry(filepaths.last().toUtf8().constData());
				}
			}
			emit this->dirSelected(item->path(), packFile->getSupportedEntryAttributes(), subdirs, subfiles);
			this->thumbnailProvider()->startProcessingThumbnails(packFile, itemPackFileParent->path(), filepaths, 128);
		} else {
			const auto fileData = packFile->readEntry(internalItemPath.toUtf8().constData());
			if (!fileData) {
				// todo: error (failed to load file)
				return;
			}
			const auto fullPath = QString{"%1/%2"}.arg(packFile->getFilename().c_str(), internalItemPath);
			emit this->fileSelected(fullPath, QByteArray::fromRawData(reinterpret_cast<const char*>(fileData->data()), static_cast<qsizetype>(fileData->size())));
		}
	});
}

const QIcon& VEntryTreeView::cachedDirIcon() {
	return ::cachedDirIcon();
}

const QIcon& VEntryTreeView::cachedFileIcon(const QString& path) {
	return ::cachedFileIcon(path);
}

/*
void VEntryTreeView::selectEntry(QString path) {
	const auto* parentItem = qobject_cast<VEntryTreeModel*>(this->model())->root();
	QModelIndex parentIndex;
	for (int i = 0; i < parentItem->childCount(); i++) {
		auto currentIndex = this->model()->index(i, 0, parentIndex);
		const auto* currentItem = static_cast<VEntryTreeItem*>(currentIndex.internalPointer());
		if (const auto searchString = currentItem->filename().indexOf('/') != -1 ? currentItem->filename() + '/' : currentItem->filename(); path.startsWith(searchString)) {
			this->setExpanded(currentIndex, true);
			path = path.sliced(searchString.length());
			i = 0;
			parentIndex = currentIndex;
			parentItem = currentItem;
		}
	}
	if (!path.isEmpty()) {
		return;
	}
	this->clearSelection();
	this->setCurrentIndex(parentIndex);
}
*/

void VEntryTreeView::addPackFile(const QString& path) const {
	dynamic_cast<VEntryTreeModel*>(this->model())->addPackFile(path);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
// NOLINTNEXTLINE(*-convert-member-functions-to-static)
void VEntryTreeView::onThemeUpdated() const {
	::cachedDirIcon() = QApplication::style()->standardIcon(STYLE_DIR_ICON);
}

VEntryTreeThumbnailProvider::VEntryTreeThumbnailProvider(QObject* parent) : QObject{parent} {
	this->processingThread = new QThread{this};
	this->processingThread->setObjectName("Thumbnail Thread");
	this->processingThread->start();

	VPluginLoader::iterate(PROJECT_NAME "_thumbnails", [this](const QString& libraryPath) {
		auto* loader = new QPluginLoader{libraryPath, this};
		if (auto* plugin = qobject_cast<IVPKEditThumbnailPlugin_V6_0*>(loader->instance())) {
			if (!loader->metaData().contains("MetaData") || !loader->metaData().value("MetaData").isObject()) {
				return;
			}
			this->thumbnailPluginLoaders.push_back(loader);

			for (const auto& ext : plugin->getFileExtensions()) {
				if (!this->thumbnailPlugins.contains(ext) || this->thumbnailPlugins[ext]->getPriority() < plugin->getPriority()) {
					this->thumbnailPlugins[ext] = plugin;
				}
			}

			// todo: provide window access
			plugin->initPlugin(nullptr);
			// todo: register the plugin somewhere in the ui
			//this->window->registerPlugin(libraryPath, plugin->getIcon(), loader->metaData()["MetaData"].toObject());
		} else {
			loader->deleteLater();
		}
	});
}

// NOLINTNEXTLINE(*-unnecessary-value-param)
void VEntryTreeThumbnailProvider::startProcessingThumbnails(std::shared_ptr<vpkpp::PackFile> packFile, QString packFilePath, QStringList paths, uint16_t size) {
	auto* worker = new VEntryTreeThumbnailProviderWorker;

	connect(worker, &VEntryTreeThumbnailProviderWorker::processedThumbnail, this, &VEntryTreeThumbnailProvider::processedThumbnail);
	connect(worker, &VEntryTreeThumbnailProviderWorker::processingComplete, this, [worker] { worker->deleteLater(); });

	worker->moveToThread(this->processingThread);
	QMetaObject::invokeMethod(worker, "processThumbnails", Qt::QueuedConnection, std::move(packFile), std::move(packFilePath), std::move(paths), size, this->thumbnailPlugins);
}

void VEntryTreeThumbnailProviderWorker::processThumbnails(const std::shared_ptr<vpkpp::PackFile>& packFile, const QString& packFilePath, const QStringList& paths, uint16_t size, const QMap<QString, IVPKEditThumbnailPlugin_V6_0*>& thumbnailPlugins) {
	this->cancelPending = false;

	if (!packFile) {
		return;
	}

	for (const auto& path : paths) {
		if (this->cancelPending) {
			break;
		}

		const auto ext = QFileInfo{path}.suffix();
		if (!thumbnailPlugins.contains(ext)) {
			continue;
		}

		const auto data = packFile->readEntry(path.toUtf8().constData());
		if (!data) {
			continue;
		}

#ifndef DEBUG
		try {
#endif
			const auto thumb = thumbnailPlugins[ext]->getThumbnail(path, reinterpret_cast<const quint8*>(data->data()), data->size());
			if (thumb.isNull()) {
				continue;
			}

			emit this->processedThumbnail(packFilePath + '/' + path, QIcon{thumb.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation)});
#ifndef DEBUG
		} catch (...) {}
#endif
	}

	emit this->processingComplete();
}

void VEntryTreeThumbnailProviderWorker::cancel() {
	this->cancelPending = true;
}
