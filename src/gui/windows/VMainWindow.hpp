#pragma once

#include <QMainWindow>

class QLineEdit;
class QSplitter;

class VEntryTreeView;
class VNavigationBar;
class VPreviewParent;

class VMainWindow : public QMainWindow {
	Q_OBJECT;

public:
	VMainWindow();

	void addPackFile(const QString& path) const;

signals:
	void themeUpdated();

protected:
	void dragEnterEvent(QDragEnterEvent* event) override;

	void dropEvent(QDropEvent* event) override;

	QLineEdit* entryTreeSearch;
	VEntryTreeView* entryTreeView;
	VNavigationBar* navigationBar;
	VPreviewParent* previewParent;
};

extern VMainWindow* g_MainWindow;
