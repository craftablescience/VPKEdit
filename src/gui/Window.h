#pragma once

#include <functional>
#include <vector>

#include <QMainWindow>
#include <vpkedit/PackFile.h>

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

    friend class CreateVPKFromDirWorker;
    friend class SavePackFileWorker;
    friend class ExtractPackFileWorker;

public:
    explicit Window(QWidget* parent = nullptr);

    void newVPK(bool fromDirectory, const QString& startPath = QString());

    void openPackFile(const QString& startPath = QString(), const QString& filePath = QString());

    void savePackFile(bool saveAs = false);

    void saveAsPackFile();

    void closePackFile();

	void checkForNewUpdate() const;

    void setProperties();

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

    [[nodiscard]] bool clearContents();

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
    QAction* openAction;
    QMenu*   openRelativeToMenu;
    QMenu*   openRecentMenu;
    QAction* saveAction;
    QAction* saveAsAction;
    QAction* closeFileAction;
    QAction* extractAllAction;
    QAction* addFileAction;
    QAction* addDirAction;
    QAction* setPropertiesAction;

    QNetworkAccessManager* checkForNewUpdateNetworkManager;

    QThread* createVPKFromDirWorkerThread;
    QThread* savePackFileWorkerThread;
    QThread* extractPackFileWorkerThread;

    std::unique_ptr<vpkedit::PackFile> packFile;
    bool modified;

    void freezeActions(bool freeze, bool freezeCreationActions = true) const;

    bool loadPackFile(const QString& path);

	void rebuildOpenRecentMenu(const QStringList& paths);

    void checkForUpdatesReply(QNetworkReply* reply);

    void writeEntryToFile(const QString& path, const vpkedit::Entry& entry);

	void resetStatusBar();
};

class CreateVPKFromDirWorker : public QObject {
	Q_OBJECT;

public:
	CreateVPKFromDirWorker() = default;

	void run(const std::string& vpkPath, const std::string& contentPath, bool saveToDir, vpkedit::PackFileOptions options);

signals:
	void taskFinished();
};

class SavePackFileWorker : public QObject {
	Q_OBJECT;

public:
	SavePackFileWorker() = default;

	void run(Window* window, const QString& savePath);

signals:
	void progressUpdated(int value);
	void taskFinished(bool success);
};

class ExtractPackFileWorker : public QObject {
    Q_OBJECT;

public:
    ExtractPackFileWorker() = default;

    void run(Window* window, const QString& saveDir, const std::function<bool(const QString&)>& predicate);

signals:
    void progressUpdated(int value);
    void taskFinished();
};
