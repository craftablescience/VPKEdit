#pragma once

#include <functional>

#include <QTreeWidget>

#include <vpkpp/vpkpp.h>

class QKeyEvent;
class QMouseEvent;
class QProgressBar;
class QThread;

class Window;

class EntryTree : public QTreeWidget {
	Q_OBJECT;

	friend class LoadPackFileWorker;

public:
	explicit EntryTree(Window* window_, QWidget* parent = nullptr);

	void loadPackFile(vpkpp::PackFile& packFile, QProgressBar* progressBar, const std::function<void()>& finishCallback);

	[[nodiscard]] bool hasEntry(const QString& path) const;

	void selectEntry(const QString& path);

	void selectSubItem(const QString& name);

	void setSearchQuery(const QString& query);

	void setAutoExpandDirectoryOnClick(bool enable);

	void removeEntryByPath(const QString& path);

	void clearContents();

	void addEntry(const QString& path);

	void extractEntries(const QStringList& paths, const QString& destination = QString());

	void createDrag(const QStringList& paths);

public slots:
	void onCurrentItemChanged(QTreeWidgetItem* item /*, QTreeWidgetItem* previous*/) const;

protected:
	void keyPressEvent(QKeyEvent* event) override;

	void mousePressEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

private:
	[[nodiscard]] QString getItemPath(QTreeWidgetItem* item) const;

	[[nodiscard]] QTreeWidgetItem* getItemAtPath(const QString& path) const;

	void addNestedEntryComponents(const QString& path) const;

	void removeEntry(QTreeWidgetItem* item);

	void removeEntryRecurse(QTreeWidgetItem* item);

	Window* window;

	QPoint dragStartPos;
	QList<QTreeWidgetItem*> dragSelectedItems;

	QThread* workerThread;
	QTreeWidgetItem* root;

	bool autoExpandDirectories;
};

class LoadPackFileWorker : public QObject {
	Q_OBJECT;

public:
	LoadPackFileWorker() = default;

	void run(EntryTree* tree, const vpkpp::PackFile& packFile);

signals:
	void progressUpdated(int value);
	void taskFinished();
};
