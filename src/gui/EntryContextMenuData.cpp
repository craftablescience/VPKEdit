#include "EntryContextMenuData.h"

EntryContextMenuData::EntryContextMenuData(bool useRoot, QWidget* parent)
		: QObject(parent) {
	this->contextMenuFile = new QMenu(parent);
	this->extractFileAction = this->contextMenuFile->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Extract File..."));
	this->contextMenuFile->addSeparator();
	this->editFileAction = this->contextMenuFile->addAction(parent->style()->standardIcon(QStyle::SP_DialogResetButton), tr("Rename/Move File..."));
	this->copyFilePathAction = this->contextMenuFile->addAction(parent->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Copy Path"));
	this->contextMenuFile->addSeparator();
	this->removeFileAction = this->contextMenuFile->addAction(parent->style()->standardIcon(QStyle::SP_TrashIcon), tr("Remove File"));

	this->contextMenuDir = new QMenu(parent);
	this->extractDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Extract Folder..."));
	this->contextMenuDir->addSeparator();
	this->addFileToDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_FileLinkIcon), tr("Add Files..."));
	this->addDirToDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_DirLinkIcon), tr("Add Folder..."));
	this->contextMenuDir->addSeparator();
	this->renameDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_DialogResetButton), tr("Rename/Move Folder..."));
	this->copyDirPathAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Copy Path"));
	this->contextMenuDir->addSeparator();
	this->removeDirAction = this->contextMenuDir->addAction(parent->style()->standardIcon(QStyle::SP_TrashIcon), tr("Remove Folder"));

	this->contextMenuSelection = new QMenu(parent);
	this->extractSelectedAction = this->contextMenuSelection->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Extract Selected..."));
	this->contextMenuSelection->addSeparator();
	this->removeSelectedAction = this->contextMenuSelection->addAction(parent->style()->standardIcon(QStyle::SP_TrashIcon), tr("Remove Selected..."));

	if (useRoot) {
		this->contextMenuAll = new QMenu(parent);
		this->extractAllAction = this->contextMenuAll->addAction(parent->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Extract All..."));
		this->contextMenuAll->addSeparator();
		this->addFileToRootAction = this->contextMenuAll->addAction(parent->style()->standardIcon(QStyle::SP_FileLinkIcon), tr("Add Files..."));
		this->addDirToRootAction = this->contextMenuAll->addAction(parent->style()->standardIcon(QStyle::SP_DirLinkIcon), tr("Add Folder..."));
	}
}

void EntryContextMenuData::setReadOnly(bool readOnly) const {
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
