#pragma once

#include <functional>

#include <QTreeWidget>

#include <vpkedit/VPK.h>

class QProgressBar;
class QThread;

class Window;

class EntryTree : public QTreeWidget {
    Q_OBJECT;

    friend class LoadVPKWorker;

public:
    explicit EntryTree(Window* window_, QWidget* parent = nullptr);

    void loadVPK(vpkedit::VPK& vpk, QProgressBar* progressBar, const std::function<void()>& finishCallback);

    void selectSubItem(const QString& name);

    void setSearchQuery(const QString& query);

    void setAutoExpandDirectoryOnClick(bool enable);

    void removeEntryByPath(const QString& path);

    void clearContents();

    void addEntry(const QString& path);

public slots:
    void onCurrentItemChanged(QTreeWidgetItem* item /*, QTreeWidgetItem* previous*/);

private:
    [[nodiscard]] QString getItemPath(QTreeWidgetItem* item);

    void addNestedEntryComponents(const QString& path);

    void removeEntry(QTreeWidgetItem* item);

    void removeEntryRecurse(QTreeWidgetItem* item);

    Window* window;

    QThread* workerThread;
    QTreeWidgetItem* root;

    bool autoExpandDirectories;
};

class LoadVPKWorker : public QObject {
    Q_OBJECT;

public:
    LoadVPKWorker() = default;

    void run(EntryTree* tree, const vpkedit::VPK& vpk);

signals:
    void progressUpdated(int value);
    void taskFinished();
};
