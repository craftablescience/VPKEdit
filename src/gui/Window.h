#pragma once

#include <functional>
#include <vector>

#include <QMainWindow>
#include <vpktool/VPK.h>

class EntryTree;
class FileViewer;

class Window : public QMainWindow {
    Q_OBJECT;

public:
    explicit Window(QWidget* parent = nullptr);

    void open();

    void open(const QString& path);

    void closeFile();

    [[nodiscard]] std::vector<std::byte> readBinaryEntry(const QString& path);

    [[nodiscard]] QString readTextEntry(const QString& path);

    void selectEntry(const QString& path);

    void extractFile(const QString& path, QString savePath = QString());

    void extractFilesIf(const QString& saveDir, const std::function<bool(const QString&)>& predicate);

    void extractDir(const QString& path, QString saveDir = QString());

    void extractAll(QString saveDir = QString());

    void clearContents();

private:
    EntryTree* entryTree;
    FileViewer* fileViewer;
    std::optional<vpktool::VPK> vpk;

    QAction* closeFileAction;
    QAction* extractAllAction;

    void writeEntryToFile(const QString& path, const vpktool::VPKEntry& entry);
};
