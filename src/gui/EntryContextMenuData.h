#pragma once

#include <QAction>
#include <QMenu>
#include <QStyle>

struct EntryContextMenuData : public QObject {
	Q_OBJECT;

public:
	explicit EntryContextMenuData(bool useRoot, QWidget* parent = nullptr);

	void setReadOnly(bool readOnly) const;

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
