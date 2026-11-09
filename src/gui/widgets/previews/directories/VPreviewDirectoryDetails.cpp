#include "VPreviewDirectoryDetails.hpp"

#include <utility>

#include <QFileInfo>
#include <QHeaderView>
#include <vpkpp/format/VPK.h>

VPreviewDirectoryDetails::VPreviewDirectoryDetails(QWidget* parent) : QTableWidget{parent} {
	this->setColumnCount(std::to_underlying(Column::COLUMN_COUNT));
	this->setColumnWidth(std::to_underlying(Column::NAME), 250);
	this->setColumnWidth(std::to_underlying(Column::TYPE), 55);
	this->setColumnWidth(std::to_underlying(Column::SIZE), 80);
	this->setColumnWidth(std::to_underlying(Column::PRELOAD_DATA_SIZE), 120);
	this->setColumnWidth(std::to_underlying(Column::ARCHIVE_INDEX), 100);
	this->setColumnWidth(std::to_underlying(Column::CRC32), 95);
	this->setColumnWidth(std::to_underlying(Column::MD5), 200);
	this->horizontalHeader()->setStretchLastSection(true);

	this->verticalHeader()->hide();
	this->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	this->setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->setSortingEnabled(true);

	this->setIconSize({24, 24});
}

void VPreviewDirectoryDetails::populate(const QString& path_, vpkpp::Attribute attrs, const QStringList& subdirs, const QMap<QString, std::optional<vpkpp::Entry>>& subfiles) {
	this->clear();
	this->setRowCount(0);

	this->path = path_;

	this->setColumnHidden(std::to_underlying(Column::SIZE),              !static_cast<bool>(attrs & vpkpp::Attribute::LENGTH));
	this->setColumnHidden(std::to_underlying(Column::PRELOAD_DATA_SIZE), !static_cast<bool>(attrs & vpkpp::Attribute::VPK_PRELOADED_DATA));
	this->setColumnHidden(std::to_underlying(Column::ARCHIVE_INDEX),     !static_cast<bool>(attrs & vpkpp::Attribute::ARCHIVE_INDEX));
	this->setColumnHidden(std::to_underlying(Column::CRC32),             !static_cast<bool>(attrs & vpkpp::Attribute::CRC32));
	this->setColumnHidden(std::to_underlying(Column::MD5),               !static_cast<bool>(attrs & vpkpp::Attribute::PCK_MD5));

	for (const auto& subdir : subdirs) {
		this->setRowCount(this->rowCount() + 1);

		this->setItem(this->rowCount() - 1, std::to_underlying(Column::NAME), new QTableWidgetItem{VEntryTreeView::cachedDirIcon(), subdir});
		this->setItem(this->rowCount() - 1, std::to_underlying(Column::TYPE), new QTableWidgetItem{tr("Folder")});

		// Need to fill the rest of the columns with an item to fix the context menu not showing up on these cells
		for (int i = std::to_underlying(Column::TYPE) + 1; i < std::to_underlying(Column::COLUMN_COUNT); i++) {
			this->setItem(this->rowCount() - 1, i, new QTableWidgetItem{""});
		}
	}

	for (const auto& [filename, entry] : subfiles.asKeyValueRange()) {
		this->setRowCount(this->rowCount() + 1);

		this->setItem(this->rowCount() - 1, std::to_underlying(Column::NAME), new QTableWidgetItem{VEntryTreeView::cachedFileIcon(filename), filename});
		this->setItem(this->rowCount() - 1, std::to_underlying(Column::TYPE), new QTableWidgetItem{QFileInfo{filename}.completeSuffix().toUpper()});

		// Bail if the entry didn't load for whatever reason
		if (!entry) {
			// Need to fill the rest of the columns with an item to fix the context menu not showing up on these cells
			for (int i = std::to_underlying(Column::TYPE) + 1; i < std::to_underlying(Column::COLUMN_COUNT); i++) {
				this->setItem(this->rowCount() - 1, i, new QTableWidgetItem{""});
			}
			continue;
		}

		if (!this->isColumnHidden(std::to_underlying(Column::SIZE))) {
			static constexpr int KB_SIZE = 1024;
			if (entry->length < KB_SIZE) {
				this->setItem(this->rowCount() - 1, std::to_underlying(Column::SIZE), new QTableWidgetItem{QString{"%1 %2"}.arg(entry->length).arg(tr("b"))});
			} else if (entry->length < KB_SIZE * KB_SIZE) {
				this->setItem(this->rowCount() - 1, std::to_underlying(Column::SIZE), new QTableWidgetItem{QString{"%1 %2"}.arg(QString::number(static_cast<double>(entry->length) / KB_SIZE, 'f', 2)).arg(tr("kb"))});
			} else if (entry->length < KB_SIZE * KB_SIZE * KB_SIZE) {
				this->setItem(this->rowCount() - 1, std::to_underlying(Column::SIZE), new QTableWidgetItem{QString{"%1 %2"}.arg(QString::number(static_cast<double>(entry->length) / KB_SIZE / KB_SIZE, 'f', 2)).arg(tr("mb"))});
			} else {
				this->setItem(this->rowCount() - 1, std::to_underlying(Column::SIZE), new QTableWidgetItem{QString{"%1 %2"}.arg(QString::number(static_cast<double>(entry->length) / KB_SIZE / KB_SIZE / KB_SIZE, 'f', 2)).arg(tr("gb"))});
			}
		}

		if (!this->isColumnHidden(std::to_underlying(Column::PRELOAD_DATA_SIZE))) {
			this->setItem(this->rowCount() - 1, std::to_underlying(Column::PRELOAD_DATA_SIZE), new QTableWidgetItem{QString{"%1 %2"}.arg(entry->extraData.size()).arg(tr("b"))});
		}

		if (!this->isColumnHidden(std::to_underlying(Column::ARCHIVE_INDEX))) {
			this->setItem(this->rowCount() - 1, std::to_underlying(Column::ARCHIVE_INDEX), new QTableWidgetItem{entry->archiveIndex == vpkpp::VPK_DIR_INDEX ? QString{"N/A"} : QString::number(entry->archiveIndex)});
		}

		if (!this->isColumnHidden(std::to_underlying(Column::CRC32))) {
			QByteArray crc32{reinterpret_cast<const char*>(&entry->crc32), sizeof(entry->crc32)};
			this->setItem(this->rowCount() - 1, std::to_underlying(Column::CRC32), new QTableWidgetItem{"0x" + crc32.toHex().toUpper()});
		}

		if (this->isColumnHidden(std::to_underlying(Column::MD5))) {
			QByteArray md5{reinterpret_cast<const char*>(entry->extraData.data()), static_cast<qsizetype>(entry->extraData.size())};
			this->setItem(this->rowCount() - 1, std::to_underlying(Column::MD5), new QTableWidgetItem{"0x" + md5.toHex().toUpper()});
		}
	}

	this->setHorizontalHeaderLabels({
		tr("Name"),
		tr("Type"),
		tr("Size"),
		tr("Preloaded Size"),
		tr("Archive Index"),
		tr("CRC32"),
		tr("MD5"),
	});
}

void VPreviewDirectoryDetails::onProcessedThumbnail(const QString& filepath, const QIcon& thumbnail) const {
	for (int i = 0; i < this->rowCount(); i++) {
		if (this->item(i, std::to_underlying(Column::TYPE))->text() == tr("Folder")) {
			continue;
		}
		if (this->path + '/' + this->item(i, std::to_underlying(Column::NAME))->text() == filepath) {
			this->item(i, std::to_underlying(Column::NAME))->setIcon(thumbnail);
		}
	}
}
