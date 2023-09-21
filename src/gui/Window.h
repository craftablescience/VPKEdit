#pragma once

#include <functional>
#include <vector>

#include <QMainWindow>
#include <vpkedit/VPK.h>

class QAction;
class QLabel;
class QLineEdit;
class QMenu;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressBar;
class QSettings;
class QThread;

class EntryTree;
class FileViewer;

class Window : public QMainWindow {
    Q_OBJECT;

    friend class ExtractVPKWorker;

public:
    explicit Window(QSettings& options, QWidget* parent = nullptr);

    void newVPK(bool fromDirectory, const QString& startPath = QString());

    void openVPK(const QString& startPath = QString());

    bool saveVPK();

    bool saveAsVPK();

    void closeVPK();

    void checkForUpdates();

    void addFile(const QString& startDir = QString());

    bool removeFile(const QString& filepath);

    void about();

    void aboutQt();

    [[nodiscard]] std::optional<std::vector<std::byte>> readBinaryEntry(const QString& path);

    [[nodiscard]] std::optional<QString> readTextEntry(const QString& path);

    void selectEntry(const QString& path);

    void selectDir(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths);

    void selectSubItemInDir(const QString& path);

    void extractFile(const QString& path, QString savePath = QString());

    void extractFilesIf(const QString& saveDir, const std::function<bool(const QString&)>& predicate);

    void extractDir(const QString& path, QString saveDir = QString());

    void extractAll(QString saveDir = QString());

    void markModified(bool modified);

    [[nodiscard]] bool promptUserToKeepModifications();

    void clearContents();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QLabel* statusText;
    QProgressBar* statusProgressBar;
    QLineEdit* searchBar;
    EntryTree* entryTree;
    FileViewer* fileViewer;

    QAction* createEmptyVPKAction;
    QAction* createVPKFromDirAction;
    QAction* openVPKAction;
    QMenu*   openVPKRelativeToMenu;
    QAction* saveVPKAction;
    QAction* saveAsVPKAction;
    QAction* closeFileAction;
    QAction* addFileAction;
    QAction* extractAllAction;

    QNetworkAccessManager* checkForUpdatesNetworkManager;

    QThread* extractWorkerThread;

    std::optional<vpkedit::VPK> vpk;
    bool modified;

    void freezeActions(bool freeze, bool freezeCreationActions = true);

    bool loadVPK(const QString& path);

    void checkForUpdatesReply(QNetworkReply* reply);

    void writeEntryToFile(const QString& path, const vpkedit::VPKEntry& entry);
};

class ExtractVPKWorker : public QObject {
    Q_OBJECT;

public:
    ExtractVPKWorker() = default;

    void run(Window* window, const QString& saveDir, const std::function<bool(const QString&)>& predicate);

signals:
    void progressUpdated(int value);
    void taskFinished();
};
