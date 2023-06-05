#pragma once

#include <functional>
#include <vector>

#include <QMainWindow>
#include <vpktool/VPK.h>

class QLabel;
class QLineEdit;
class QProgressBar;

class EntryTree;
class FileViewer;

class Window : public QMainWindow {
    Q_OBJECT;

public:
    explicit Window(QWidget* parent = nullptr);

    void open();

    bool open(const QString& path);

    void closeFile();

    [[nodiscard]] std::vector<std::byte> readBinaryEntry(const QString& path);

    [[nodiscard]] QString readTextEntry(const QString& path);

    void selectEntry(const QString& path);

    void selectDir(const QList<QString>& subfolders, const QList<QString>& entryPaths);

    void extractFile(const QString& path, QString savePath = QString());

    void extractFilesIf(const QString& saveDir, const std::function<bool(const QString&)>& predicate);

    void extractDir(const QString& path, QString saveDir = QString());

    void extractAll(QString saveDir = QString());

    void clearContents();

private:
    QLabel* statusText;
    QProgressBar* statusProgressBar;
    QLineEdit* searchBar;
    EntryTree* entryTree;
    FileViewer* fileViewer;
    std::optional<vpktool::VPK> vpk;

    QAction* closeFileAction;
    QAction* extractAllAction;

    void writeEntryToFile(const QString& path, const vpktool::VPKEntry& entry);
};
