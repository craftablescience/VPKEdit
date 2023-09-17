#pragma once

#include <functional>

#include <QTreeWidget>
#include <vpktool/VPK.h>

class QProgressBar;
class QThread;

class Window;

class EntryTree : public QTreeWidget {
    Q_OBJECT;

    friend class LoadVPKWorker;

public:
    explicit EntryTree(Window* window_, QWidget* parent = nullptr);

    void loadVPK(vpktool::VPK& vpk, QProgressBar* progressBar, const std::function<void()>& finishCallback);

    void selectSubItem(const QString& name);

    void setSearchQuery(const QString& query);

    void setAutoExpandDirectoryOnClick(bool enable);

    void clearContents();

public slots:
    void onItemClicked(QTreeWidgetItem* item, int /*column*/);

private:
    [[nodiscard]] QString getItemPath(QTreeWidgetItem* item);

    void removeEntry(QTreeWidgetItem* item);

    Window* window;

    QThread* workerThread;
    QTreeWidgetItem* root;

    bool autoExpandDirectories;
};

class LoadVPKWorker : public QObject {
    Q_OBJECT;

public:
    LoadVPKWorker() = default;

    void run(EntryTree* tree, const vpktool::VPK& vpk);

signals:
    void progressUpdated(int value);
    void taskFinished();
};
