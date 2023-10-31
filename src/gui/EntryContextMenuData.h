#pragma once

#include <type_traits>

#include <QAction>
#include <QMenu>
#include <QStyle>

struct EntryContextMenuData {
    explicit EntryContextMenuData(bool useRoot, QWidget* parent = nullptr) {
        this->contextMenuFile = new QMenu(parent);
        this->extractFileAction = this->contextMenuFile->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), QObject::tr("Extract File"));
        this->contextMenuFile->addSeparator();
        this->removeFileAction = this->contextMenuFile->addAction(parent->style()->standardIcon(QStyle::SP_TrashIcon), QObject::tr("Remove File"));

        this->contextMenuDir = new QMenu(parent);
        this->extractDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), QObject::tr("Extract Folder"));
        this->contextMenuDir->addSeparator();
        this->addFileToDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_FileIcon), QObject::tr("Add File..."));
        this->removeDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_TrashIcon), QObject::tr("Remove Folder"));

        if (useRoot) {
            this->contextMenuAll = new QMenu(parent);
            this->extractAllAction = this->contextMenuAll->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), QObject::tr("Extract All"));
            this->contextMenuAll->addSeparator();
            this->addFileToRootAction = this->contextMenuAll->addAction(parent->style()->standardIcon(QStyle::SP_FileIcon), QObject::tr("Add File..."));
        }
    }

    QMenu* contextMenuFile = nullptr;
    QAction* extractFileAction = nullptr;
    QAction* removeFileAction = nullptr;

    QMenu* contextMenuDir = nullptr;
    QAction* extractDirAction = nullptr;
    QAction* addFileToDirAction = nullptr;
    QAction* removeDirAction = nullptr;

    QMenu* contextMenuAll = nullptr;
    QAction* extractAllAction = nullptr;
    QAction* addFileToRootAction = nullptr;
};
