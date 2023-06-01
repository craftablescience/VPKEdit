#pragma once

#include <QTreeWidget>
#include <vpktool/VPK.h>

class Window;

class EntryTree : public QTreeWidget {
    Q_OBJECT;

public:
    explicit EntryTree(Window* window_, QWidget* parent = nullptr);

    void loadVPK(vpktool::VPK& vpk);

    void clearContents();

public slots:
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/);

private:
    [[nodiscard]] static QString getItemPath(QTreeWidgetItem* item);

    Window* window;
};
