#include "EntryTree.h"

#include <QMenu>
#include <QProgressBar>
#include <QStyle>
#include <QThread>

#include "Window.h"

using namespace vpktool;

EntryTree::EntryTree(Window* window_, QWidget* parent)
        : QTreeWidget(parent)
        , window(window_) {
    this->setMinimumWidth(200);
    this->setHeaderHidden(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    this->workerThread = nullptr;
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

    connect(this, &QTreeWidget::itemClicked, this, &EntryTree::onItemClicked);

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
    connect(this->workerThread, &QThread::started, worker, [this, worker, &vpk] {
        worker->run(this, vpk);
    });
    connect(worker, &LoadVPKWorker::progressUpdated, this, [progressBar] {
        progressBar->setValue(progressBar->value() + 1);
    });
    connect(worker, &LoadVPKWorker::taskFinished, this, [this, finishCallback] {
        // Kill thread
        this->workerThread->quit();
        this->workerThread->wait();
        delete this->workerThread;
        this->workerThread = nullptr;

        // Ok we've loaded let them touch it
        this->setDisabled(false);
        this->root->setDisabled(false);

        // Fire the click manually to show root contents
        this->root->setSelected(true);
        this->onItemClicked(this->root, 0);

        finishCallback();
    });
    workerThread->start();
}

void EntryTree::setSearchQuery(const QString& query) {
    for (QTreeWidgetItemIterator it(this); *it; ++it) {
        QTreeWidgetItem* item = (*it);
        item->setHidden(false);
        if (item->childCount() == 0 && !item->text(0).contains(query, Qt::CaseInsensitive)) {
            item->setHidden(true);
        }
    }
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

void EntryTree::clearContents() {
    this->root = nullptr;
    this->clear();
}

void EntryTree::onItemClicked(QTreeWidgetItem* item, int /*column*/) {
    item->setExpanded(!item->isExpanded());

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
        this->window->selectDir(subfolders, entryPaths);
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

void LoadVPKWorker::run(EntryTree* tree, const VPK& vpk) {
    int progress = 0;
    for (const auto& [directory, entries] : vpk.getEntries()) {
        emit progressUpdated(++progress);
        for (const auto& entry : entries) {
            QStringList components = (QString(directory.c_str()) + '/' + entry.filename.c_str()).split('/', Qt::SkipEmptyParts);
            QTreeWidgetItem* currentItem = nullptr;

            for (const auto& component : components) {
                QTreeWidgetItem* newItem = nullptr;

                // Find the child item with the current component text under the current parent item
                int childCount = currentItem ? currentItem->childCount() : tree->root->childCount();
                for (int i = 0; i < childCount; ++i) {
                    QTreeWidgetItem* childItem = currentItem ? currentItem->child(i) : tree->root->child(i);
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
                        newItem = new QTreeWidgetItem(tree->root);
                    }
                    newItem->setText(0, component);
                }

                currentItem = newItem;
            }
        }
    }
    tree->sortItems(0, Qt::AscendingOrder);
    emit taskFinished();
}
