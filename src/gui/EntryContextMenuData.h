#pragma once

#include <QAction>
#include <QMenu>
#include <QStyle>

struct EntryContextMenuData {
	explicit EntryContextMenuData(bool useRoot, QWidget* parent = nullptr) {
		this->contextMenuFile = new QMenu(parent);
		this->extractFileAction = this->contextMenuFile->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), QObject::tr("Extract File..."));
		this->contextMenuFile->addSeparator();
		this->editFileAction = this->contextMenuFile->addAction(parent->style()->standardIcon(QStyle::SP_DialogResetButton), QObject::tr("Rename/Move File..."));
		this->copyFilePathAction = this->contextMenuFile->addAction(parent->style()->standardIcon(QStyle::SP_FileDialogDetailedView), QObject::tr("Copy Path"));
		this->contextMenuFile->addSeparator();
		this->removeFileAction = this->contextMenuFile->addAction(parent->style()->standardIcon(QStyle::SP_TrashIcon), QObject::tr("Remove File"));

		this->contextMenuDir = new QMenu(parent);
		this->extractDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), QObject::tr("Extract Folder..."));
		this->contextMenuDir->addSeparator();
		this->addFileToDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_FileLinkIcon), QObject::tr("Add File..."));
		this->addDirToDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_DirLinkIcon), QObject::tr("Add Folder..."));
		this->contextMenuDir->addSeparator();
		this->renameDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_DialogResetButton), QObject::tr("Rename/Move Folder..."));
		this->copyDirPathAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_FileDialogDetailedView), QObject::tr("Copy Path"));
		this->contextMenuDir->addSeparator();
		this->removeDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_TrashIcon), QObject::tr("Remove Folder"));

		this->contextMenuSelection = new QMenu(parent);
		this->extractSelectedAction = this->contextMenuSelection->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), QObject::tr("Extract Selected..."));
		this->contextMenuSelection->addSeparator();
		this->removeSelectedAction = this->contextMenuSelection->addAction(parent->style()->standardIcon(QStyle::SP_TrashIcon), QObject::tr("Remove Selected..."));

		if (useRoot) {
			this->contextMenuAll = new QMenu(parent);
			this->extractAllAction = this->contextMenuAll->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), QObject::tr("Extract All..."));
			this->contextMenuAll->addSeparator();
			this->addFileToRootAction = this->contextMenuAll->addAction(parent->style()->standardIcon(QStyle::SP_FileLinkIcon), QObject::tr("Add File..."));
			this->addDirToRootAction = this->contextMenuAll->addAction(parent->style()->standardIcon(QStyle::SP_DirLinkIcon), QObject::tr("Add Folder..."));
		}
	}

	void setReadOnly(bool readOnly) const {
		this->editFileAction->setDisabled(readOnly);
		this->removeFileAction->setDisabled(readOnly);

		this->addFileToDirAction->setDisabled(readOnly);
		this->addDirToDirAction->setDisabled(readOnly);
		this->renameDirAction->setDisabled(readOnly);
		this->removeDirAction->setDisabled(readOnly);

		this->removeSelectedAction->setDisabled(readOnly);

		if (this->addFileToRootAction) {
			this->addFileToRootAction->setDisabled(readOnly);
		}
		if (this->addDirToRootAction) {
			this->addDirToRootAction->setDisabled(readOnly);
		}
	}

	QMenu* contextMenuFile = nullptr;
	QAction* extractFileAction = nullptr;
	QAction* editFileAction = nullptr;
	QAction* copyFilePathAction = nullptr;
	QAction* removeFileAction = nullptr;

	QMenu* contextMenuDir = nullptr;
	QAction* extractDirAction = nullptr;
	QAction* addFileToDirAction = nullptr;
	QAction* addDirToDirAction = nullptr;
	QAction* renameDirAction = nullptr;
	QAction* copyDirPathAction = nullptr;
	QAction* removeDirAction = nullptr;

	QMenu* contextMenuSelection = nullptr;
	QAction* extractSelectedAction = nullptr;
	QAction* removeSelectedAction = nullptr;

	QMenu* contextMenuAll = nullptr;
	QAction* extractAllAction = nullptr;
	QAction* addFileToRootAction = nullptr;
	QAction* addDirToRootAction = nullptr;
};
