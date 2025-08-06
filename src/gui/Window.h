#pragma once

#include <functional>
#include <vector>

#include <QDir>
#include <QMainWindow>
#include <vpkpp/vpkpp.h>

#include "dialogs/PackFileOptionsDialog.h"

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

	friend class SavePackFileWorker;
	friend class ExtractPackFileWorker;

public:
	explicit Window(QWidget* parent = nullptr);

	void newPackFile(std::string_view typeGUID, bool fromDirectory, const QString& startPath, const QString& name, const QString& extension);

	void newBMZ(bool fromDirectory, const QString& startPath = QString());

	void newFPX(bool fromDirectory, const QString& startPath = QString());

	void newPAK(bool fromDirectory, const QString& startPath = QString());

	void newPCK(bool fromDirectory, const QString& startPath = QString());

	void newVPK(bool fromDirectory, const QString& startPath = QString());

	void newVPK_VTMB(bool fromDirectory, const QString& startPath = QString());

	void newWAD3(bool fromDirectory, const QString& startPath = QString());

	void newZIP(bool fromDirectory, const QString& startPath = QString());

	void openDir(const QString& startPath = QString(), const QString& dirPath = QString());

	void openPackFile(const QString& startPath = QString(), const QString& filePath = QString());

	void savePackFile(bool saveAs = false, bool async = true);

	void saveAsPackFile(bool async = true);

	void closePackFile();

	void checkForNewUpdate(bool hidden = false) const;

	[[nodiscard]] bool isReadOnly() const;

	void setProperties();

	void addFile(bool showOptions, const QString& startDir = QString(), const QString& filePath = QString());

	void addFiles(bool showOptions, const QString& startDir = QString());

	void addDir(bool showOptions, const QString& startDir = QString(), const QString& dirPath = QString());

	bool removeFile(const QString& path);

	void removeDir(const QString& path) const;

	void requestEntryRemoval(const QString& path) const;

	void editFile(const QString& oldPath);

	void editFileContents(const QString& path, std::vector<std::byte> data);

	void editFileContents(const QString& path, const QString& data);

	void encryptFile(const QString& path);

	void decryptFile(const QString& path);

	void renameDir(const QString& oldPath, const QString& newPath_ = QString());

	void generateKeyPairFiles(const QString& name = QString());

	void signPackFile(const QString& privateKeyLocation = QString());

	[[nodiscard]] std::optional<std::vector<std::byte>> readBinaryEntry(const QString& path) const;

	[[nodiscard]] std::optional<QString> readTextEntry(const QString& path) const;

	void selectEntryInEntryTree(const QString& path) const;

	void selectEntryInFileViewer(const QString& path) const;

	void selectDirInFileViewer(const QString& path, const QList<QString>& subfolders, const QList<QString>& entryPaths) const;

	[[nodiscard]] bool hasEntry(const QString& path) const;

	void selectSubItemInDir(const QString& path) const;

	void extractFile(const QString& path, QString savePath = QString());

	void extractFilesIf(const std::function<bool(const QString&)>& predicate, const QString& savePath = QString());

	void extractDir(const QString& path, const QString& saveDir = QString());

	void extractPaths(const QStringList& paths, const QString& saveDir = QString());

	void createDrag(const QStringList& paths);

	void extractAll(QString saveDir = QString());

	void setDropEnabled(bool dropEnabled_);

	void markModified(bool modified_);

	[[nodiscard]] bool promptUserToKeepModifications();

	[[nodiscard]] bool clearContents();

	void freezeActions(bool freeze, bool freezeCreationActions = true, bool freezeFileViewer = true) const;

	void freezeModifyActions(bool readOnly) const;

protected:
	void mousePressEvent(QMouseEvent* event) override;

	void dragEnterEvent(QDragEnterEvent* event) override;

	void dropEvent(QDropEvent* event) override;

	void closeEvent(QCloseEvent* event) override;

signals:
	void themeUpdated();

private:
	QLabel* statusText;
	QProgressBar* statusProgressBar;
	QLineEdit* searchBar;
	EntryTree* entryTree;
	FileViewer* fileViewer;

	QMenu*   createEmptyMenu;
	QMenu*   createFromDirMenu;
	QAction* openAction;
	QAction* openDirAction;
	QMenu*   openRelativeToMenu;
	QMenu*   openRecentMenu;
	QAction* saveAction;
	QAction* saveAsAction;
	QAction* closeFileAction;
	QAction* extractAllAction;
	QAction* addFileAction;
	QAction* addDirAction;
	QAction* setPropertiesAction;
	QMenu*   toolsGeneralMenu;
	QMenu*   toolsVPKMenu;

	QNetworkAccessManager* checkForNewUpdateNetworkManager;

	QThread* createPackFileFromDirWorkerThread = nullptr;
	QThread* savePackFileWorkerThread          = nullptr;
	QThread* extractPackFileWorkerThread       = nullptr;
	QThread* scanSteamGamesWorkerThread        = nullptr;

	std::unique_ptr<vpkpp::PackFile> packFile;
	PackFileOptions packFileOptions;

	bool dropEnabled;

	bool loadDir(const QString& path);

	bool loadPackFile(const QString& path);

	bool loadPackFile(const QString& path, std::unique_ptr<vpkpp::PackFile>&& newPackFile);

	void rebuildOpenInMenu();

	void rebuildOpenRecentMenu(const QStringList& paths);

	void checkForUpdatesReply(QNetworkReply* reply);

	bool writeEntryToFile(const QString& entryPath, const QString& filepath);

	void resetStatusBar();
};

class IndeterminateProgressWorker : public QObject {
	Q_OBJECT;

public:
	IndeterminateProgressWorker() = default;

	void run(const std::function<void()>& fn);

signals:
	void taskFinished();
};

class SavePackFileWorker : public QObject {
	Q_OBJECT;

public:
	SavePackFileWorker() = default;

	void run(Window* window, const QString& savePath, vpkpp::BakeOptions options, bool async = true);

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
	void taskFinished(bool success);
};

class ScanSteamGamesWorker : public QObject {
	Q_OBJECT;

public:
	ScanSteamGamesWorker() = default;

	void run();

signals:
	void taskFinished(const QList<std::tuple<QString, QIcon, QDir>>& sourceGames);
};
