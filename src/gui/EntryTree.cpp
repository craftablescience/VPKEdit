#include "EntryTree.h"

#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProgressBar>
#include <QStyle>
#include <QThread>

#include "config/Options.h"
#include "previews/TextPreview.h"
#include "EntryContextMenuData.h"
#include "Window.h"

#if defined(_WIN32)
	#include <windows.h>
#endif

using namespace vpkedit;

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

} // namespace

class EntryItem : public QTreeWidgetItem {
public:
	explicit EntryItem(QTreeWidget* parent)
			: QTreeWidgetItem(parent) {}

	explicit EntryItem(QTreeWidgetItem* parent)
			: QTreeWidgetItem(parent) {}

	bool operator<(const QTreeWidgetItem& other) const override {
		if (!this->childCount() && other.childCount()) {
			return false;
		}
		if (this->childCount() && !other.childCount()) {
			return true;
		}
		return QTreeWidgetItem::operator<(other);
	}
};

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
    QObject::connect(this, &QTreeWidget::customContextMenuRequested, this, [this, contextMenuData](const QPoint& pos) {
		contextMenuData.setReadOnly(this->window->isReadOnly());
        if (auto* selectedItem = this->itemAt(pos)) {
            QString path = this->getItemPath(selectedItem);
            if (path.isEmpty()) {
                // Show the root context menu at the requested position
                auto* selectedAllAction = contextMenuData.contextMenuAll->exec(this->mapToGlobal(pos));

                // Handle the selected action
                if (selectedAllAction == contextMenuData.extractAllAction) {
                    this->window->extractAll();
                } else if (selectedAllAction == contextMenuData.addFileToRootAction) {
                    this->window->addFile(false);
                } else if (selectedAllAction == contextMenuData.addDirToRootAction) {
                    this->window->addDir(false);
                }
            } else if (selectedItem->childCount() > 0) {
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
                    this->removeEntry(selectedItem);
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
                    this->removeEntry(selectedItem);
                }
            }
        }
    });

    QObject::connect(this, &QTreeWidget::currentItemChanged, this, &EntryTree::onCurrentItemChanged);

    this->clearContents();
}

void EntryTree::loadPackFile(PackFile& packFile, QProgressBar* progressBar, const std::function<void()>& finishCallback) {
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
    QObject::connect(worker, &LoadPackFileWorker::taskFinished, this, [this, finishCallback] {
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
                newItem = new EntryItem(currentItem);
            } else {
                newItem = new EntryItem(this->root);
            }
            newItem->setText(0, components[i]);

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
    for (const auto& [directory, entries] : packFile.getBakedEntries()) {
        emit progressUpdated(++progress);
        for (const auto& entry : entries) {
            tree->addNestedEntryComponents(QString(directory.c_str()) + '/' + entry.getFilename().c_str());
        }
    }
    tree->sortItems(0, Qt::AscendingOrder);
    emit taskFinished();
}
