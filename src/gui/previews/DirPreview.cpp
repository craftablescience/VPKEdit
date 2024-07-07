#include "DirPreview.h"

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMessageBox>
#include <QtGlobal>
#include <vpkpp/format/VPK.h>

#include "../EntryContextMenuData.h"
#include "../FileViewer.h"
#include "../Window.h"

using namespace vpkpp;

constexpr int KB_SIZE = 1024;

constexpr const char* DIR_TYPE_NAME = "Folder";

namespace Column {

enum Column : int {
	NAME = 0,
	TYPE,
	LENGTH,
	VPK_PRELOADED_DATA_LENGTH,
	VPK_ARCHIVE_INDEX,
	CRC32,
	PCK_MD5,
	COLUMN_COUNT,
};

} // namespace Columns

namespace {

QString attributeToQString(Attribute attribute) {
	switch (attribute) {
		using enum Attribute;
		case LENGTH:
			return QObject::tr("Size");
		case CRC32:
			return QObject::tr("CRC32");
		case PCK_MD5:
			return QObject::tr("MD5");
		case ARCHIVE_INDEX:
			return QObject::tr("Archive Index");
		case VPK_PRELOADED_DATA_LENGTH:
			return QObject::tr("Preloaded Size");
		default:
			break;
	}
	return QObject::tr("Unknown");
}

void hideUnsupportedAttributes(DirPreview* dirPreview, const PackFile& packFile) {
	auto attributes = packFile.getSupportedEntryAttributes();
	for (int i = 0; i < static_cast<int>(Attribute::ATTRIBUTE_COUNT); i++) {
		dirPreview->setColumnHidden(i + 2, std::find(attributes.begin(), attributes.end(), static_cast<Attribute>(i)) == attributes.end());
	}

	// dunno why the fuck this has to be here and not in the ctor but okay then
	QStringList header{QObject::tr("Name"), QObject::tr("Type")};
	for (int i = 0; i < static_cast<int>(Attribute::ATTRIBUTE_COUNT); i++) {
		header.append(::attributeToQString(static_cast<Attribute>(i)));
	}
	dirPreview->setHorizontalHeaderLabels(header);
}

} // namespace

DirPreview::DirPreview(FileViewer* fileViewer_, Window* window_, QWidget* parent)
		: QTableWidget(parent)
		, fileViewer(fileViewer_)
		, window(window_) {
	this->setColumnCount(static_cast<int>(Attribute::ATTRIBUTE_COUNT) + 2);
	this->setColumnWidth(Column::NAME, 250);
	this->setColumnWidth(Column::TYPE, 55);
	this->setColumnWidth(Column::LENGTH, 80);
	this->setColumnWidth(Column::VPK_PRELOADED_DATA_LENGTH, 90);
	this->setColumnWidth(Column::VPK_ARCHIVE_INDEX, 100);
	this->setColumnWidth(Column::CRC32, 95);
	this->setColumnWidth(Column::PCK_MD5, 200);
	this->horizontalHeader()->setStretchLastSection(true);

	this->verticalHeader()->hide();
	this->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	this->setEditTriggers(QAbstractItemView::NoEditTriggers);

	this->setContextMenuPolicy(Qt::CustomContextMenu);
	EntryContextMenuData contextMenuData(false, this);
	QObject::connect(this, &QTableWidget::customContextMenuRequested, this, [this, contextMenuData](const QPoint& pos) {
		contextMenuData.setReadOnly(this->window->isReadOnly());
		if (this->selectedItems().length() > this->columnCount()) {
			// Show the selection context menu at the requested position
			auto* selectedSelectionAction = contextMenuData.contextMenuSelection->exec(this->mapToGlobal(pos));

			// Handle the selected action
			if (selectedSelectionAction == contextMenuData.extractSelectedAction) {
				QStringList paths;
				for (auto* item : this->selectedItems()) {
					paths.push_back(this->getItemPath(item));
				}
				this->window->extractPaths(paths);
			} else if (selectedSelectionAction == contextMenuData.removeSelectedAction) {
				this->removeSelectedRows(false);
			}
		} else if (auto* selectedItem = this->itemAt(pos)) {
			QString path = this->getItemPath(selectedItem);
			if (this->item(selectedItem->row(), Column::TYPE)->text() == DIR_TYPE_NAME) {
				// Show the directory context menu at the requested position
				auto* selectedDirAction = contextMenuData.contextMenuDir->exec(this->mapToGlobal(pos));

				// Handle the selected action
				if (selectedDirAction == contextMenuData.extractDirAction) {
					this->window->extractDir(path);
				} else if (selectedDirAction == contextMenuData.addFileToDirAction) {
					this->window->addFile(false, path);
				} else if (selectedDirAction == contextMenuData.addDirToDirAction) {
					this->window->addDir(false, path);
				} else if (selectedDirAction == contextMenuData.renameDirAction) {
					this->window->renameDir(path);
				} else if (selectedDirAction == contextMenuData.copyDirPathAction) {
					QGuiApplication::clipboard()->setText(path);
				} else if (selectedDirAction == contextMenuData.removeDirAction) {
					this->window->requestEntryRemoval(path);
					this->removeDir(path);
				}
			} else {
				// Show the file context menu at the requested position
				auto* selectedFileAction = contextMenuData.contextMenuFile->exec(this->mapToGlobal(pos));

				// Handle the selected action
				if (selectedFileAction == contextMenuData.extractFileAction) {
					this->window->extractFile(path);
				} else if (selectedFileAction == contextMenuData.editFileAction) {
					this->window->editFile(path);
				} else if (selectedFileAction == contextMenuData.copyFilePathAction) {
					QGuiApplication::clipboard()->setText(path);
				} else if (selectedFileAction == contextMenuData.removeFileAction) {
					this->window->requestEntryRemoval(path);
				}
			}
		}
	});

	QObject::connect(this, &QTableWidget::doubleClicked, this, [this](const QModelIndex& index) {
		this->fileViewer->selectSubItemInDir(this->item(index.row(), Column::NAME)->text());
	});
}

void DirPreview::setPath(const QString& currentDir, const QList<QString>& subfolders, const QList<QString>& entryPaths, const PackFile& packFile) {
	this->clear();
	this->setRowCount(0);

	::hideUnsupportedAttributes(this, packFile);

	this->currentPath = currentDir;

	for (const auto& subfolder : subfolders) {
		this->addRowForDir(subfolder);
	}
	for (const auto& path : entryPaths) {
		this->addRowForFile(packFile, path);
	}

	// Make sure the active search query is applied
	this->setSearchQuery(this->currentSearchQuery);
}

void DirPreview::addEntry(const PackFile& packFile, const QString& path) {
	// If the parent is identical to us then we're the direct parent, add the file
	const auto lastIndex = path.lastIndexOf('/');
	const QString parent = lastIndex < 0 ? "" : path.sliced(0, lastIndex);
	if (parent == this->currentPath) {
		this->addRowForFile(packFile, path);
		return;
	}

	// We may need to add a subfolder if the displayed directory is a grandparent or higher
	if (path.length() <= this->currentPath.length() || !path.startsWith(this->currentPath)) {
		return;
	}

	// Check this subfolder doesn't already exist
	QString subfolderName = path.sliced(this->currentPath.length());
	if (subfolderName.isEmpty()) {
		return;
	}
	if (subfolderName.startsWith('/')) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
		subfolderName.removeFirst();
#else
		subfolderName.remove(0, 1);
#endif
	}
	auto subFolderLastIndex = subfolderName.indexOf('/');
	if (subFolderLastIndex < 0) {
		subFolderLastIndex = subfolderName.size();
	}
	subfolderName = subfolderName.sliced(0, subFolderLastIndex);
	bool exists = false;
	for (int r = 0; r < this->rowCount(); r++) {
		if (this->item(r, Column::TYPE)->text() != DIR_TYPE_NAME) {
			continue;
		}
		if (this->item(r, Column::NAME)->text() == subfolderName) {
			exists = true;
			break;
		}
	}
	if (!exists) {
		this->addRowForDir(subfolderName);
	}
}

void DirPreview::removeFile(const QString& path) {
	// If the parent isn't identical to us, bail
	const auto lastIndex = path.lastIndexOf('/');
	const QString parent = lastIndex < 0 ? "" : path.sliced(0, lastIndex);
	if (parent != this->currentPath) {
		return;
	}
	QString name = lastIndex < 0 ? path : path.sliced(path.lastIndexOf('/') + 1);
	for (int r = 0; r < this->rowCount(); r++) {
		if (this->item(r, Column::TYPE)->text() == DIR_TYPE_NAME) {
			continue;
		}
		if (this->item(r, Column::NAME)->text() == name) {
			this->removeRow(r);
			break;
		}
	}
}

void DirPreview::removeDir(const QString& path) {
	// If the parent isn't identical to us, bail
	const auto lastIndex = path.lastIndexOf('/');
	const QString parent = lastIndex < 0 ? "" : path.sliced(0, lastIndex);
	if (parent != this->currentPath) {
		return;
	}
	QString name = lastIndex < 0 ? path : path.sliced(path.lastIndexOf('/') + 1);
	for (int r = 0; r < this->rowCount(); r++) {
		if (this->item(r, Column::TYPE)->text() != DIR_TYPE_NAME) {
			continue;
		}
		if (this->item(r, Column::NAME)->text() == name) {
			this->removeRow(r);
			break;
		}
	}
}

void DirPreview::setSearchQuery(const QString& query) {
	// Copy query to use when resetting the preview
	this->currentSearchQuery = query;

	// If the query is empty, show everything
	if (query.isEmpty()) {
		for (int r = 0; r < this->rowCount(); r++) {
			this->showRow(r);
		}
		return;
	}

	// Set items that contain a word in the query visible
	const auto words = query.split(' ');
	for (int r = 0; r < this->rowCount(); r++) {
		this->showRow(r);

		// todo(search): use the entry tree visibility status to inform which directories should be shown
		if (this->item(r, Column::TYPE)->text() == DIR_TYPE_NAME) {
			continue;
		}

		for (const auto& word: words) {
			if (!this->item(r, Column::NAME)->text().contains(word, Qt::CaseInsensitive)) {
				this->hideRow(r);
			}
		}
	}
}

const QString& DirPreview::getCurrentPath() const {
	return this->currentPath;
}

void DirPreview::keyPressEvent(QKeyEvent* event) {
	if (event->keyCombination().key() == Qt::Key_Delete) {
		event->accept();
		this->removeSelectedRows(event->keyCombination().keyboardModifiers() != Qt::SHIFT);
	}
	QTableWidget::keyPressEvent(event);
}

void DirPreview::mousePressEvent(QMouseEvent* event) {
	this->dragStartPos = event->pos();
	this->dragSelectedItems = this->selectedItems();

	QTableWidget::mousePressEvent(event);
}

void DirPreview::mouseMoveEvent(QMouseEvent* event) {
	if (!(event->buttons() & Qt::LeftButton)) {
		return QTableWidget::mouseMoveEvent(event);
	}
	if ((event->pos() - this->dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
		return QTableWidget::mouseMoveEvent(event);
	}
	if (this->dragSelectedItems.isEmpty()) {
		return QTableWidget::mouseMoveEvent(event);
	}
	event->accept();

	QStringList paths;
	for (auto* item : this->dragSelectedItems) {
		paths.push_back(this->getItemPath(item));
	}
	this->window->createDrag(paths);
}

void DirPreview::addRowForFile(const PackFile& packFile, const QString& path) {
	// Note: does not check if the path is inside the directory being previewed
	auto entry = packFile.findEntry(path.toLocal8Bit().constData());
	if (!entry) {
		return;
	}

	this->setRowCount(this->rowCount() + 1);

	// NAME
	auto* nameItem = new QTableWidgetItem(entry->getFilename().c_str());
	this->setItem(this->rowCount() - 1, Column::NAME, nameItem);

	// TYPE
	auto* typeItem = new QTableWidgetItem(QString(entry->getExtension().c_str()).toUpper());
	this->setItem(this->rowCount() - 1, Column::TYPE, typeItem);

	// LENGTH
	QTableWidgetItem* sizeItem;
	if (entry->length < KB_SIZE) {
		sizeItem = new QTableWidgetItem(QString::number(entry->length) + ' ' + tr("bytes"));
	} else {
		auto size = static_cast<double>(entry->length) / KB_SIZE;
		QString extension(' ' + tr("kb"));

		if (size >= KB_SIZE) {
			size /= KB_SIZE;
			extension = ' ' + tr("mb");
		}
		if (size >= KB_SIZE) {
			size /= KB_SIZE;
			extension = ' ' + tr("gb");
		}
		sizeItem = new QTableWidgetItem(QString::number(size, 'f', 2) + extension);
	}
	this->setItem(this->rowCount() - 1, Column::LENGTH, sizeItem);

	// PRELOADED DATA LENGTH
	auto* preloadedSizeItem = new QTableWidgetItem(QString::number(entry->vpk_preloadedData.size()) + ' ' + tr("bytes"));
	this->setItem(this->rowCount() - 1, Column::VPK_PRELOADED_DATA_LENGTH, preloadedSizeItem);

	// ARCHIVE INDEX
	auto archiveIndex = entry->archiveIndex;
	// If the archive index is the dir index, it's included in the directory VPK
	auto* archiveIndexItem = new QTableWidgetItem(archiveIndex == VPK_DIR_INDEX ? QString("N/A") : QString::number(archiveIndex));
	this->setItem(this->rowCount() - 1, Column::VPK_ARCHIVE_INDEX, archiveIndexItem);

	// CRC32
	QByteArray crc32{reinterpret_cast<const char*>(&entry->crc32), sizeof(entry->crc32)};
	auto* crc32Item = new QTableWidgetItem("0x" + crc32.toHex().toUpper());
	this->setItem(this->rowCount() - 1, Column::CRC32, crc32Item);

	// MD5
	QByteArray md5{reinterpret_cast<const char*>(entry->pck_md5.data()), static_cast<qsizetype>(entry->pck_md5.size())};
	auto* md5Item = new QTableWidgetItem("0x" + md5.toHex().toUpper());
	this->setItem(this->rowCount() - 1, Column::PCK_MD5, md5Item);
}

void DirPreview::addRowForDir(const QString& name) {
	this->setRowCount(this->rowCount() + 1);

	auto* nameItem = new QTableWidgetItem(name);
	this->setItem(this->rowCount() - 1, Column::NAME, nameItem);

	auto* typeItem = new QTableWidgetItem(DIR_TYPE_NAME);
	this->setItem(this->rowCount() - 1, Column::TYPE, typeItem);

	// Need to fill the rest of the columns with an item to fix the context menu not showing up on these cells
	for (int i = Column::TYPE + 1; i < Column::COLUMN_COUNT; i++) {
		this->setItem(this->rowCount() - 1, i, new QTableWidgetItem(""));
	}
}

QString DirPreview::getItemPath(QTableWidgetItem* item) const {
	QString entryName = this->item(item->row(), Column::NAME)->text();
	if (currentPath.isEmpty()) {
		return entryName;
	}
	return this->currentPath + '/' + entryName;
}

void DirPreview::removeSelectedRows(bool needsConfirmDialog) {
	QList<QTableWidgetItem*> selectedRows;
	for (auto* item : this->selectedItems()) {
		bool foundMatch = false;
		for (auto* selectedAlready : selectedRows) {
			if (item->row() == selectedAlready->row()) {
				foundMatch = true;
				break;
			}
		}
		if (!foundMatch) {
			selectedRows.push_back(item);
		}
	}
	for (auto* item : selectedRows) {
		const auto path = this->getItemPath(item);
		if (needsConfirmDialog) {
			auto reply = QMessageBox::question(this, tr("Delete Entry"), tr("Are you sure you want to delete \"%1\"?\n(Hold Shift to skip this popup.)").arg(path), QMessageBox::Ok | QMessageBox::Cancel);
			if (reply == QMessageBox::Cancel) {
				return;
			}
		}
		bool wasDir = this->item(item->row(), Column::TYPE)->text() == DIR_TYPE_NAME;
		this->window->requestEntryRemoval(this->getItemPath(item));
		if (wasDir) {
			this->removeRow(item->row());
		}
	}
}
