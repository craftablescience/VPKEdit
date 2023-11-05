#include "EntryTree.h"

#include <QMenu>
#include <QProgressBar>
#include <QStyle>
#include <QThread>

#include "EntryContextMenuData.h"
#include "Window.h"

using namespace vpkedit;

EntryTree::EntryTree(Window* window_, QWidget* parent)
        : QTreeWidget(parent)
        , window(window_)
        , autoExpandDirectories(false) {
    this->setMinimumWidth(200);
    this->setHeaderHidden(true);

    this->workerThread = nullptr;
    this->root = nullptr;

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    EntryContextMenuData contextMenuData(true, this);
    QObject::connect(this, &QTreeWidget::customContextMenuRequested, [=](const QPoint& pos) {
        if (auto* selectedItem = this->itemAt(pos)) {
            QString path = this->getItemPath(selectedItem);
            if (path.isEmpty()) {
                // Show the root context menu at the requested position
                auto* selectedAllAction = contextMenuData.contextMenuAll->exec(this->mapToGlobal(pos));

                // Handle the selected action
                if (selectedAllAction == contextMenuData.addFileToRootAction) {
                    this->window->addFile();
                } else if (selectedAllAction == contextMenuData.extractAllAction) {
                    this->window->extractAll();
                }
            } else if (selectedItem->childCount() > 0) {
                // Show the directory context menu at the requested position
                auto* selectedDirAction = contextMenuData.contextMenuDir->exec(this->mapToGlobal(pos));

                // Handle the selected action
                if (selectedDirAction == contextMenuData.addFileToDirAction) {
                    this->window->addFile(path);
                } else if (selectedDirAction == contextMenuData.removeDirAction) {
                    this->removeEntry(selectedItem);
                } else if (selectedDirAction == contextMenuData.extractDirAction) {
                    this->window->extractDir(path);
                }
            } else {
                // Show the directory context menu at the requested position
                auto* selectedFileAction = contextMenuData.contextMenuFile->exec(this->mapToGlobal(pos));

                // Handle the selected action
                if (selectedFileAction == contextMenuData.removeFileAction) {
                    this->removeEntry(selectedItem);
                } else if (selectedFileAction == contextMenuData.extractFileAction) {
                    this->window->extractFile(path);
                }
            }
        }
    });

    QObject::connect(this, &QTreeWidget::itemClicked, this, &EntryTree::onItemClicked);

    this->clearContents();
}

void EntryTree::loadVPK(VPK& vpk, QProgressBar* progressBar, const std::function<void()>& finishCallback) {
    this->root = new QTreeWidgetItem(this);
    this->root->setText(0, vpk.getPrettyFileName().data());

    // Set up progress bar
    progressBar->setMinimum(0);
    progressBar->setMaximum(static_cast<int>(vpk.getEntries().size()));
    progressBar->setValue(0);

    // Don't let the user touch anything
    this->setDisabled(true);
    this->root->setDisabled(true);

    // Set up thread
    this->workerThread = new QThread(this);
    auto* worker = new LoadVPKWorker();
    worker->moveToThread(this->workerThread);
    QObject::connect(this->workerThread, &QThread::started, worker, [this, worker, &vpk] {
        worker->run(this, vpk);
    });
    QObject::connect(worker, &LoadVPKWorker::progressUpdated, this, [progressBar](int value) {
        progressBar->setValue(value);
    });
    QObject::connect(worker, &LoadVPKWorker::taskFinished, this, [this, finishCallback] {
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
        this->onItemClicked(this->root, 0);
        this->root->setExpanded(true);

        finishCallback();
    });
    this->workerThread->start();
}

void EntryTree::selectSubItem(const QString& name) {
    for (auto* selected : this->selectedItems()) {
        selected->setSelected(false);
        selected->setExpanded(true);
        for (int i = 0; i < selected->childCount(); ++i) {
            auto* child = selected->child(i);
            if (child->text(0) == name) {
                child->setSelected(true);
                this->onItemClicked(child, 0);
                child->setExpanded(true);
            }
        }
    }
}

void EntryTree::setSearchQuery(const QString& query) {
    // Set items that contain a word in the query visible
    const auto words = query.split(' ');
    for (QTreeWidgetItemIterator it(this); *it; ++it) {
        QTreeWidgetItem* item = (*it);
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
            QTreeWidgetItem* item = (*it);
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

void EntryTree::clearContents() {
    this->root = nullptr;
    this->clear();
}

void EntryTree::addEntry(const QString& path) {
    this->addNestedEntryComponents(path);
}

void EntryTree::onItemClicked(QTreeWidgetItem* item, int /*column*/) {
    if (this->autoExpandDirectories) {
        item->setExpanded(!item->isExpanded());
    }

    auto path = this->getItemPath(item);
    if (item->childCount() == 0) {
        this->window->selectEntry(path);
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
        this->window->selectDir(path, subfolders, entryPaths);
    }
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

void EntryTree::addNestedEntryComponents(const QString& path) {
    QStringList components = path.split('/', Qt::SkipEmptyParts);
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

void EntryTree::removeEntry(QTreeWidgetItem* item) {
    auto* parent = item->parent();
    this->removeEntryRecurse(item);

    // Remove dead directories
    while (parent && parent != this->root && parent->childCount() == 0) {
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

void LoadVPKWorker::run(EntryTree* tree, const VPK& vpk) {
    int progress = 0;
    for (const auto& [directory, entries] : vpk.getEntries()) {
        emit progressUpdated(++progress);
        for (const auto& entry : entries) {
            tree->addNestedEntryComponents(QString(directory.c_str()) + '/' + entry.filename.c_str());
        }
    }
    tree->sortItems(0, Qt::AscendingOrder);
    emit taskFinished();
}
