#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <QTreeView>
#include <vpkpp/vpkpp.h>

class QKeyEvent;
class QMouseEvent;
class QProgressBar;
class QThread;

class Window;

class EntryTreeNode {
	friend class EntryTreeModel;

public:
	EntryTreeNode(EntryTreeNode* parent, QString name, bool isDirectory = false);

	[[nodiscard]] EntryTreeNode* findChild(const QString& name) const;

	void sort(Qt::SortOrder order);

	[[nodiscard]] const EntryTreeNode* parent() const;

	[[nodiscard]] const QString& name() const;

	void setName(QString name);

	[[nodiscard]] const std::vector<std::unique_ptr<EntryTreeNode>>& children() const;

	[[nodiscard]] bool isDirectory() const;

protected:
	[[nodiscard]] EntryTreeNode* parent();

	[[nodiscard]] std::vector<std::unique_ptr<EntryTreeNode>>& children();

	EntryTreeNode* parent_;
	QString name_;
	std::vector<std::unique_ptr<EntryTreeNode>> children_;
	bool isDirectory_;
};

class EntryTreeModel : public QAbstractItemModel {
	Q_OBJECT;

	friend class EntryTree;

public:
	explicit EntryTreeModel(QObject* parent = nullptr);

	[[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent) const override;

	[[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;

	[[nodiscard]] int rowCount(const QModelIndex& parent) const override;

	[[nodiscard]] int columnCount(const QModelIndex&) const override;

	[[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;

	[[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;

	void clear();

	void addEntry(const QString& path, bool sort = true);

	void removeEntry(const QString& path);

	[[nodiscard]] bool hasEntry(const QString& path) const;

	[[nodiscard]] EntryTreeNode* getNodeAtPath(const QString& path) const;

	[[nodiscard]] QString getNodePath(const EntryTreeNode* node) const;

	void sort(int column, Qt::SortOrder order) override;

	[[nodiscard]] const EntryTreeNode* root() const;

	[[nodiscard]] EntryTreeNode* root();

protected:
	[[nodiscard]] QModelIndex getIndexAtNode(const EntryTreeNode* node) const;

	[[nodiscard]] static EntryTreeNode* getNodeAtIndex(const QModelIndex& index);

	std::unique_ptr<EntryTreeNode> root_;
};

class EntryTree : public QTreeView {
	Q_OBJECT;

	friend class LoadPackFileWorker;

public:
	explicit EntryTree(Window* window_, QWidget* parent = nullptr);

	void loadPackFile(vpkpp::PackFile& packFile, QProgressBar* progressBar, const std::function<void()>& finishCallback);

	[[nodiscard]] bool hasEntry(const QString& path) const;

	void selectEntry(const QString& path);

	void selectSubItem(const QString& name);

	void setSearchQuery(const QString& query) const;

	void setAutoExpandDirectoryOnClick(bool enable);

	void removeEntryByPath(const QString& path) const;

	void clearContents() const;

	void addEntry(const QString& path, bool sort = true) const;

	void extractEntries(const QStringList& paths, const QString& destination = QString());

	void createDrag(const QStringList& paths);

protected:
	void keyPressEvent(QKeyEvent* event) override;

	void mousePressEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

private slots:
	void onCurrentIndexChanged(const QModelIndex& index);

private:
	[[nodiscard]] QString getIndexPath(const QModelIndex& index) const;

	Window* window;

	EntryTreeModel* model;

	QPoint dragStartPos;
	QList<QModelIndex> dragSelectedIndexes;

	QThread* workerThread;

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
