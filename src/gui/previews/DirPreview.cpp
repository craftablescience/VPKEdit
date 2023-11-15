#include "DirPreview.h"

#include <QHeaderView>

#include <vpkedit/VPK.h>

#include "../EntryContextMenuData.h"
#include "../FileViewer.h"
#include "../Window.h"

using namespace vpkedit;

constexpr int KB_SIZE = 1024;

constexpr const char* DIR_TYPE_NAME = "Folder";

namespace DirPreviewColumn {

constexpr int NAME = 0;
constexpr int TYPE = 1;
constexpr int TOTAL_SIZE = 2;
constexpr int PRELOADED_SIZE = 3;
constexpr int ARCHIVE_INDEX = 4;

constexpr int NUM_COLUMNS = 5;

} // namespace DirPreviewColumn

DirPreview::DirPreview(FileViewer* fileViewer_, Window* window_, QWidget* parent)
        : QTableWidget(parent)
        , fileViewer(fileViewer_)
        , window(window_) {
    this->setColumnCount(DirPreviewColumn::NUM_COLUMNS);
    this->setColumnWidth(DirPreviewColumn::NAME, 250);
    this->setColumnWidth(DirPreviewColumn::TYPE, 50);
    this->setColumnWidth(DirPreviewColumn::TOTAL_SIZE, 100);
    this->setColumnWidth(DirPreviewColumn::PRELOADED_SIZE, 100);
    this->setColumnWidth(DirPreviewColumn::ARCHIVE_INDEX, 100);
    this->horizontalHeader()->setStretchLastSection(true);
    this->verticalHeader()->hide();
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    EntryContextMenuData contextMenuData(false, this);
    QObject::connect(this, &QTableWidget::customContextMenuRequested, [=](const QPoint& pos) {
        if (auto* selectedItem = this->itemAt(pos)) {
            QString path = this->getItemPath(selectedItem);
            if (this->item(selectedItem->row(), DirPreviewColumn::TYPE)->text() == DIR_TYPE_NAME) {
                // Show the directory context menu at the requested position
                auto* selectedDirAction = contextMenuData.contextMenuDir->exec(this->mapToGlobal(pos));

                // Handle the selected action
                if (selectedDirAction == contextMenuData.extractDirAction) {
                    this->window->extractDir(path);
                } else if (selectedDirAction == contextMenuData.addFileToDirAction) {
                    this->window->addFile(false, path);
                } else if (selectedDirAction == contextMenuData.addDirToDirAction) {
                    this->window->addDir(false, path);
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
                } else if (selectedFileAction == contextMenuData.removeFileAction) {
                    this->window->requestEntryRemoval(path);
                }
            }
        }
    });

    QObject::connect(this, &QTableWidget::doubleClicked, [=](const QModelIndex& index) {
        this->fileViewer->selectSubItemInDir(this->item(index.row(), DirPreviewColumn::NAME)->text());
    });
}

void DirPreview::setPath(const QString& currentDir, const QList<QString>& subfolders, const QList<QString>& entryPaths, const VPK& vpk) {
    this->clear();
    this->setRowCount(0);
    this->setHorizontalHeaderLabels({"Name", "Type", "Size", "Preloaded Size", "Archive Index"});

    this->currentPath = currentDir;

    for (const auto& subfolder : subfolders) {
        this->addRowForDir(subfolder);
    }
    for (const auto& path : entryPaths) {
        this->addRowForFile(vpk, path);
    }

    // Make sure the active search query is applied
    this->setSearchQuery(this->currentSearchQuery);
}

void DirPreview::addEntry(const vpkedit::VPK& vpk, const QString& path) {
    // If the parent is identical to us then we're the direct parent, add the file
    const auto lastIndex = path.lastIndexOf('/');
    const QString parent = lastIndex < 0 ? "" : path.sliced(0, lastIndex);
    if (parent == this->currentPath) {
        this->addRowForFile(vpk, path);
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
		subfolderName.removeFirst();
	}
    subfolderName = subfolderName.sliced(0, subfolderName.indexOf('/'));
    bool exists = false;
    for (int r = 0; r < this->rowCount(); r++) {
        if (this->item(r, DirPreviewColumn::TYPE)->text() != DIR_TYPE_NAME) {
            continue;
        }
        if (this->item(r, DirPreviewColumn::NAME)->text() == subfolderName) {
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
        if (this->item(r, DirPreviewColumn::TYPE)->text() == DIR_TYPE_NAME) {
            continue;
        }
        if (this->item(r, DirPreviewColumn::NAME)->text() == name) {
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
        if (this->item(r, DirPreviewColumn::TYPE)->text() != DIR_TYPE_NAME) {
            continue;
        }
        if (this->item(r, DirPreviewColumn::NAME)->text() == name) {
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
        if (this->item(r, DirPreviewColumn::TYPE)->text() == DIR_TYPE_NAME) {
            continue;
        }

        for (const auto& word: words) {
            if (!this->item(r, DirPreviewColumn::NAME)->text().contains(word, Qt::CaseInsensitive)) {
                this->hideRow(r);
            }
        }
    }
}

const QString& DirPreview::getCurrentPath() const {
    return this->currentPath;
}

void DirPreview::addRowForFile(const VPK& vpk, const QString& path) {
    // Note: does not check if the path is inside the directory being previewed
    auto entry = vpk.findEntry(path.toStdString());
    if (!entry) {
        return;
    }

    this->setRowCount(this->rowCount() + 1);

    auto* nameItem = new QTableWidgetItem(entry->filename.c_str());
    this->setItem(this->rowCount() - 1, DirPreviewColumn::NAME, nameItem);

    auto* typeItem = new QTableWidgetItem(QString(entry->filenamePair.second.c_str()).toUpper());
    this->setItem(this->rowCount() - 1, DirPreviewColumn::TYPE, typeItem);

    QTableWidgetItem* sizeItem;
    if (entry->length < KB_SIZE) {
        sizeItem = new QTableWidgetItem(QString::number(entry->length) + " bytes");
    } else {
        auto size = static_cast<double>(entry->length) / KB_SIZE;
        QString extension(" kb");

        if (size >= KB_SIZE) {
            size /= KB_SIZE;
            extension = " mb";
        }
        if (size >= KB_SIZE) {
            size /= KB_SIZE;
            extension = " gb";
        }
        sizeItem = new QTableWidgetItem(QString::number(size, 'f', 2) + extension);
    }
    this->setItem(this->rowCount() - 1, DirPreviewColumn::TOTAL_SIZE, sizeItem);

    auto* preloadedSizeItem = new QTableWidgetItem(QString::number(entry->preloadedData.size()) + " bytes");
    this->setItem(this->rowCount() - 1, DirPreviewColumn::PRELOADED_SIZE, preloadedSizeItem);

    auto archiveIndex = entry->archiveIndex;
    // If the archive index is the dir index, it's included in the directory VPK
    auto* archiveIndexItem = new QTableWidgetItem(archiveIndex == VPK_DIR_INDEX ? QString("N/A") : QString::number(archiveIndex));
    this->setItem(this->rowCount() - 1, DirPreviewColumn::ARCHIVE_INDEX, archiveIndexItem);
}

void DirPreview::addRowForDir(const QString& name) {
    this->setRowCount(this->rowCount() + 1);

    auto* nameItem = new QTableWidgetItem(name);
    this->setItem(this->rowCount() - 1, DirPreviewColumn::NAME, nameItem);

    auto* typeItem = new QTableWidgetItem(DIR_TYPE_NAME);
    this->setItem(this->rowCount() - 1, DirPreviewColumn::TYPE, typeItem);

    // Need to fill the rest of the columns with an item to fix the context menu not showing up on these cells
    this->setItem(this->rowCount() - 1, DirPreviewColumn::TOTAL_SIZE, new QTableWidgetItem(""));
    this->setItem(this->rowCount() - 1, DirPreviewColumn::PRELOADED_SIZE, new QTableWidgetItem(""));
    this->setItem(this->rowCount() - 1, DirPreviewColumn::ARCHIVE_INDEX, new QTableWidgetItem(""));
}

QString DirPreview::getItemPath(QTableWidgetItem* item) const {
    QString entryName = this->item(item->row(), DirPreviewColumn::NAME)->text();
    if (currentPath.isEmpty()) {
        return entryName;
    }
    return this->currentPath + '/' + entryName;
}
