#include "VPreviewDirectoryCompact.hpp"

VPreviewDirectoryCompact::VPreviewDirectoryCompact(QWidget* parent) : QListWidget{parent} {
	this->setResizeMode(QListView::Adjust);
	this->setWrapping(true);
	this->setCompactOrIcons(true);
}

void VPreviewDirectoryCompact::populate(const QString& path_, vpkpp::Attribute, const QStringList& subdirs, const QMap<QString, std::optional<vpkpp::Entry>>& subfiles) {
	this->clear();

	this->path = path_;
	for (const auto& subdir : subdirs) {
		auto* item = new QListWidgetItem{this};
		item->setText(subdir);
		item->setIcon(VEntryTreeView::cachedDirIcon());
		this->addItem(item);
	}
	for (const auto& [filename, entry] : subfiles.asKeyValueRange()) {
		auto* item = new QListWidgetItem{this};
		item->setText(filename);
		item->setIcon(VEntryTreeView::cachedFileIcon(filename));
		this->addItem(item);
	}
	this->resizeItems();
}

bool VPreviewDirectoryCompact::isCompact() const {
	return this->viewMode() == QListView::ListMode;
}

bool VPreviewDirectoryCompact::isIcons() const {
	return this->viewMode() == QListView::IconMode;
}

void VPreviewDirectoryCompact::setCompactOrIcons(bool icons) {
	this->setViewMode(icons ? QListView::IconMode : QListView::ListMode);
	this->setFlow(icons ? QListView::LeftToRight : QListView::TopToBottom);
	this->resizeItems();
}

void VPreviewDirectoryCompact::onProcessedThumbnail(const QString& filepath, const QIcon& thumbnail) const {
	for (int i = 0; i < this->count(); i++) {
		if (auto* item = this->item(i); filepath == this->path + '/' + item->text()) {
			item->setIcon(thumbnail);
			this->viewport()->update();
			return;
		}
	}
}

void VPreviewDirectoryCompact::resizeItems() {
	if (this->isCompact()) {
		this->setIconSize({48, 48});
		this->setGridSize({this->sizeHintForColumn(0), this->sizeHintForRow(0) + 2});
		this->setSpacing(4);
	} else {
		this->setIconSize({96, 96});
		this->setGridSize({this->sizeHintForColumn(0), this->sizeHintForRow(0) + 12});
		this->setSpacing(12);
	}
}
