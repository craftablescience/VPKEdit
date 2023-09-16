#pragma once

#include <functional>
#include <vector>

#include <QMainWindow>
#include <vpktool/VPK.h>

class QLabel;
class QLineEdit;
class QProgressBar;
class QSettings;

class EntryTree;
class FileViewer;

class Window : public QMainWindow {
    Q_OBJECT;

public:
    explicit Window(QSettings& options, QWidget* parent = nullptr);

    void newVPK(const QString& startPath = QString());

    void openVPK(const QString& startPath = QString());

    bool saveVPK();

    bool saveAsVPK();

    void closeVPK();

    static void checkForUpdates();

    void addFile();

    void about();

    void aboutQt();

    [[nodiscard]] std::vector<std::byte> readBinaryEntry(const QString& path);

    [[nodiscard]] QString readTextEntry(const QString& path);

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

    QAction* saveVPKAction;
    QAction* saveAsVPKAction;
    QAction* closeFileAction;
    QAction* addFileAction;
    QAction* extractAllAction;

    std::optional<vpktool::VPK> vpk;
    bool modified;

    bool loadVPK(const QString& path);

    void writeEntryToFile(const QString& path, const vpktool::VPKEntry& entry);
};
