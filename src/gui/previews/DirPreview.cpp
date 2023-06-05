#include "DirPreview.h"

#include "../Window.h"

#include <QHeaderView>

using namespace vpktool;

DirPreview::DirPreview(QWidget* parent)
        : QTableWidget(parent) {
    this->setColumnCount(5);
    this->setColumnWidth(0, 250);
    this->setColumnWidth(1, 50);
    this->setColumnWidth(2, 100);
    this->setColumnWidth(3, 100);
    this->setColumnWidth(4, 100);
    this->horizontalHeader()->setStretchLastSection(true);
    this->verticalHeader()->hide();
    this->setSelectionMode(QAbstractItemView::NoSelection);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void DirPreview::setPath(const QList<QString>& subfolders, const QList<QString>& entryPaths, const VPK& vpk) {
    this->clear();
    this->setRowCount(0);
    this->setHorizontalHeaderLabels({"Name", "Type", "Size", "Preloaded Size", "Archive Index"});

    for (const auto& subfolder : subfolders) {
        this->setRowCount(this->rowCount() + 1);

        auto* nameItem = new QTableWidgetItem(subfolder);
        this->setItem(this->rowCount() - 1, 0, nameItem);

        auto* typeItem = new QTableWidgetItem(QString("Folder"));
        this->setItem(this->rowCount() - 1, 1, typeItem);
    }

    for (const auto& path : entryPaths) {
        auto entry = vpk.findEntry(path.toStdString());
        if (!entry) {
            continue;
        }

        this->setRowCount(this->rowCount() + 1);

        auto* nameItem = new QTableWidgetItem(entry->filename.c_str());
        this->setItem(this->rowCount() - 1, 0, nameItem);

        auto* typeItem = new QTableWidgetItem(QString(entry->filename.c_str()).split(".")[1].toUpper());
        this->setItem(this->rowCount() - 1, 1, typeItem);

        auto size = static_cast<double>(entry->length);
        QString extension(" bytes");
        if (size >= 1024) {
            size /= 1024.0;
            extension = " kb";
        }
        if (size >= 1024) {
            size /= 1024.0;
            extension = " mb";
        }
        if (size >= 1024) {
            size /= 1024.0;
            extension = " gb";
        }
        auto* sizeItem = new QTableWidgetItem(QString::number(size, 'f', 2) + extension);
        this->setItem(this->rowCount() - 1, 2, sizeItem);

        auto* preloadedSizeItem = new QTableWidgetItem(QString::number(entry->preloadedData.size()) + " bytes");
        this->setItem(this->rowCount() - 1, 3, preloadedSizeItem);

        auto archiveIndex = entry->archiveIndex;
        // If the archive index is 0x7fff, it's included in the directory VPK
        auto* archiveIndexItem = new QTableWidgetItem(archiveIndex == 0x7fff ? QString("N/A") : QString::number(archiveIndex));
        this->setItem(this->rowCount() - 1, 4, archiveIndexItem);
    }
}
