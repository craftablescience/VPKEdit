#include "EntryTree.h"

#include <QMenu>
#include <QStyle>

#include "Window.h"

using namespace vpktool;

EntryTree::EntryTree(Window* window_, QWidget* parent)
        : QTreeWidget(parent)
        , window(window_) {
    this->setHeaderHidden(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    this->root = nullptr;

    auto* contextMenuFile = new QMenu(this);
    auto* extractFileAction = contextMenuFile->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Extract File"));
    auto* contextMenuDir = new QMenu(this);
    auto* extractDirAction = contextMenuDir->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Extract Folder"));
    auto* contextMenuAll = new QMenu(this);
    auto* extractAllAction = contextMenuAll->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Extract All"));

    connect(this, &QTreeWidget::customContextMenuRequested,
            [&, contextMenuFile, extractFileAction, contextMenuDir, extractDirAction, contextMenuAll, extractAllAction](const QPoint& pos) {
        if (auto* selectedItem = this->itemAt(pos)) {
            QString path = this->getItemPath(selectedItem);
            if (path.isEmpty()) {
                // Show the root context menu at the requested position
                auto* selectedAllAction = contextMenuAll->exec(this->mapToGlobal(pos));

                // Handle the selected action
                if (selectedAllAction == extractAllAction) {
                    this->window->extractAll();
                }
            } else if (selectedItem->childCount() > 0) {
                // Show the directory context menu at the requested position
                auto* selectedDirAction = contextMenuDir->exec(this->mapToGlobal(pos));

                // Handle the selected action
                if (selectedDirAction == extractDirAction) {
                    this->window->extractDir(path);
                }
            } else {
                // Show the directory context menu at the requested position
                auto* selectedFileAction = contextMenuFile->exec(this->mapToGlobal(pos));

                // Handle the selected action
                if (selectedFileAction == extractFileAction) {
                    this->window->extractFile(path);
                }
            }
        }
    });

    connect(this, &QTreeWidget::currentItemChanged, this, &EntryTree::onCurrentItemChanged);

    this->clearContents();
}

void EntryTree::loadVPK(VPK& vpk) {
    this->root = new QTreeWidgetItem(this);
    this->root->setText(0, vpk.getPrettyFileName().data());
    this->root->setExpanded(true);

    for (const auto& [directory, entries] : vpk.getEntries()) {
        for (const auto& entry : entries) {
            QStringList components = (QString(directory.c_str()) + '/' + entry.filename.c_str()).split('/', Qt::SkipEmptyParts);
            QTreeWidgetItem* currentItem = nullptr;

            for (const auto& component : components) {
                QTreeWidgetItem* newItem = nullptr;

                // Find the child item with the current component text under the current parent item
                int childCount = currentItem ? currentItem->childCount() : this->root->childCount();
                for (int i = 0; i < childCount; ++i) {
                    QTreeWidgetItem* childItem = currentItem ? currentItem->child(i) : this->root->child(i);
                    if (childItem->text(0) == component) {
                        newItem = childItem;
                        break;
                    }
                }

                // If the child item doesn't exist, create a new one
                if (!newItem) {
                    if (currentItem) {
                        newItem = new QTreeWidgetItem(currentItem);
                    } else {
                        newItem = new QTreeWidgetItem(this->root);
                    }
                    newItem->setText(0, component);
                }

                currentItem = newItem;
            }
        }
    }
    this->sortItems(0, Qt::AscendingOrder);
}

void EntryTree::clearContents() {
    this->root = nullptr;
    this->clear();
}

void EntryTree::onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/) {
    if (!current || current->childCount() != 0) {
        return;
    }
    auto path = this->getItemPath(current);
    this->window->selectEntry(path);
}

QString EntryTree::getItemPath(QTreeWidgetItem* item) {
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
