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

    friend class CreateFromDirVPKWorker;
    friend class SaveVPKWorker;
    friend class ExtractVPKWorker;

public:
    explicit Window(QWidget* parent = nullptr);

    void newVPK(bool fromDirectory, const QString& startPath = QString());

    void openVPK(const QString& startPath = QString(), const QString& filePath = QString());

    void saveVPK(bool saveAs = false);

    void saveAsVPK();

    void closeVPK();

	void checkForNewUpdate() const;

    void changeVPKVersion();

    void addFile(bool showOptions, const QString& startDir = QString(), const QString& filePath = QString());

    void addDir(bool showOptions, const QString& startDir = QString(), const QString& dirPath = QString());

    bool removeFile(const QString& path);

    void removeDir(const QString& path) const;

    void requestEntryRemoval(const QString& path) const;

    void editFile(const QString& oldPath);

    void renameDir(const QString& oldPath, const QString& newPath_ = QString());

    void about();

    void aboutQt();

    void controls();

    [[nodiscard]] std::optional<std::vector<std::byte>> readBinaryEntry(const QString& path) const;

    [[nodiscard]] std::optional<QString> readTextEntry(const QString& path) const;

	void selectEntryInEntryTree(const QString& path) const;

    void selectEntryInFileViewer(const QString& path) const;

    void selectDirInFileViewer(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths) const;

	[[nodiscard]] bool hasEntry(const QString& path) const;

    void selectSubItemInDir(const QString& path) const;

    void extractFile(const QString& path, QString savePath = QString());

    void extractFilesIf(const QString& saveDir, const std::function<bool(const QString&)>& predicate);

    void extractDir(const QString& path, QString saveDir = QString());

    void extractAll(QString saveDir = QString());

    void markModified(bool modified_);

    [[nodiscard]] bool promptUserToKeepModifications();

    void clearContents();

protected:
	void mousePressEvent(QMouseEvent* event) override;

	void dragEnterEvent(QDragEnterEvent* event) override;

	void dropEvent(QDropEvent* event) override;

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
    QAction* extractAllAction;
    QAction* addFileAction;
    QAction* addDirAction;
    QAction* changeVersionAction;

    QNetworkAccessManager* checkForNewUpdateNetworkManager;

    QThread* createFromDirWorkerThread;
    QThread* saveWorkerThread;
    QThread* extractWorkerThread;

    std::optional<vpkedit::VPK> vpk;
    bool modified;

    void freezeActions(bool freeze, bool freezeCreationActions = true) const;

    bool loadVPK(const QString& path);

    void checkForUpdatesReply(QNetworkReply* reply);

    void writeEntryToFile(const QString& path, const vpkedit::VPKEntry& entry);

	void resetStatusBar();
};

class CreateFromDirVPKWorker : public QObject {
	Q_OBJECT;

public:
	CreateFromDirVPKWorker() = default;

	void run(const std::string& vpkPath, const std::string& contentPath, bool saveToDir, vpkedit::VPKOptions options);

signals:
	void taskFinished();
};

class SaveVPKWorker : public QObject {
	Q_OBJECT;

public:
	SaveVPKWorker() = default;

	void run(Window* window, const QString& savePath);

signals:
	void progressUpdated(int value);
	void taskFinished(bool success);
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
