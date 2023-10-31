#include "DirPreview.h"

#include <QHeaderView>
#include <QMenu>

#include <vpkedit/VPK.h>

#include "../EntryContextMenuData.h"
#include "../FileViewer.h"
#include "../Window.h"

using namespace vpkedit;

constexpr const char* DIR_TYPE_NAME = "Folder";

DirPreview::DirPreview(FileViewer* fileViewer_, Window* window_, QWidget* parent)
        : QTableWidget(parent)
        , fileViewer(fileViewer_)
        , window(window_) {
    this->setColumnCount(5);
    this->setColumnWidth(0, 250);
    this->setColumnWidth(1, 50);
    this->setColumnWidth(2, 100);
    this->setColumnWidth(3, 100);
    this->setColumnWidth(4, 100);
    this->horizontalHeader()->setStretchLastSection(true);
    this->verticalHeader()->hide();
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    EntryContextMenuData contextMenuData(false, this);
    QObject::connect(this, &QTableWidget::customContextMenuRequested, [=](const QPoint& pos) {
        if (auto* selectedItem = this->itemAt(pos)) {
            QString path = this->getItemPath(selectedItem);
            if (this->item(selectedItem->row(), 1)->text() == DIR_TYPE_NAME) {
                // Show the directory context menu at the requested position
                auto* selectedDirAction = contextMenuData.contextMenuDir->exec(this->mapToGlobal(pos));

                // Handle the selected action
                if (selectedDirAction == contextMenuData.addFileToDirAction) {
                    this->window->addFile(path);
                } else if (selectedDirAction == contextMenuData.removeDirAction) {
                    // todo: remove entry from dir preview
                    //this->removeEntry(selectedItem);
                } else if (selectedDirAction == contextMenuData.extractDirAction) {
                    this->window->extractDir(path);
                }
            } else {
                // Show the directory context menu at the requested position
                auto* selectedFileAction = contextMenuData.contextMenuFile->exec(this->mapToGlobal(pos));

                // Handle the selected action
                if (selectedFileAction == contextMenuData.removeFileAction) {
                    // todo: remove entry from dir preview
                    //this->removeEntry(selectedItem);
                } else if (selectedFileAction == contextMenuData.extractFileAction) {
                    this->window->extractFile(path);
                }
            }
        }
    });

    QObject::connect(this, &QTableWidget::doubleClicked, [=](const QModelIndex& index) {
        this->fileViewer->selectSubItemInDir(this->item(index.row(), 0)->text());
    });
}

void DirPreview::setPath(const QString& currentDir, const QList<QString>& subfolders, const QList<QString>& entryPaths, const VPK& vpk) {
    this->clear();
    this->setRowCount(0);
    this->setHorizontalHeaderLabels({"Name", "Type", "Size", "Preloaded Size", "Archive Index"});

    this->currentPath = currentDir;

    for (const auto& subfolder : subfolders) {
        this->setRowCount(this->rowCount() + 1);

        auto* nameItem = new QTableWidgetItem(subfolder);
        this->setItem(this->rowCount() - 1, 0, nameItem);

        auto* typeItem = new QTableWidgetItem(QString(DIR_TYPE_NAME));
        this->setItem(this->rowCount() - 1, 1, typeItem);

        // Need to fill the rest of the columns with an item to fix the context menu not showing up on these cells
        this->setItem(this->rowCount() - 1, 2, new QTableWidgetItem(""));
        this->setItem(this->rowCount() - 1, 3, new QTableWidgetItem(""));
        this->setItem(this->rowCount() - 1, 4, new QTableWidgetItem(""));
    }

    for (const auto& path : entryPaths) {
        auto entry = vpk.findEntry(path.toStdString());
        if (!entry) {
            continue;
        }

        this->setRowCount(this->rowCount() + 1);

        auto* nameItem = new QTableWidgetItem(entry->filename.c_str());
        this->setItem(this->rowCount() - 1, 0, nameItem);

        auto* typeItem = new QTableWidgetItem(QString(entry->filenamePair.second.c_str()).toUpper());
        this->setItem(this->rowCount() - 1, 1, typeItem);

        QTableWidgetItem* sizeItem;
        if (entry->length < 1024) {
            sizeItem = new QTableWidgetItem(QString::number(entry->length) + " bytes");
        } else {
            auto size = static_cast<double>(entry->length) / 1024.0;
            QString extension(" kb");

            if (size >= 1024) {
                size /= 1024.0;
                extension = " mb";
            }
            if (size >= 1024) {
                size /= 1024.0;
                extension = " gb";
            }
            sizeItem = new QTableWidgetItem(QString::number(size, 'f', 2) + extension);
        }
        this->setItem(this->rowCount() - 1, 2, sizeItem);

        auto* preloadedSizeItem = new QTableWidgetItem(QString::number(entry->preloadedData.size()) + " bytes");
        this->setItem(this->rowCount() - 1, 3, preloadedSizeItem);

        auto archiveIndex = entry->archiveIndex;
        // If the archive index is the dir index, it's included in the directory VPK
        auto* archiveIndexItem = new QTableWidgetItem(archiveIndex == VPK_DIR_INDEX ? QString("N/A") : QString::number(archiveIndex));
        this->setItem(this->rowCount() - 1, 4, archiveIndexItem);
    }
}

QString DirPreview::getItemPath(QTableWidgetItem* item) {
    QString entryName = this->item(item->row(), 0)->text();
    if (currentPath.isEmpty()) {
        return entryName;
    }
    return this->currentPath + '/' + entryName;
}
