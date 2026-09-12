#include "VPreviewDirectoryParent.hpp"

#include "directories/VPreviewDirectoryCompact.hpp"
#include "directories/VPreviewDirectoryDetails.hpp"

VPreviewDirectoryParent::VPreviewDirectoryParent(QWidget* parent) : QStackedWidget{parent} {
	this->addWidget(this->previewCompact = new VPreviewDirectoryCompact{this});
	this->addWidget(this->previewDetails = new VPreviewDirectoryDetails{this});
}

void VPreviewDirectoryParent::populate(const QString& path, vpkpp::Attribute attrs, const QStringList& subdirs, const QMap<QString, std::optional<vpkpp::Entry>>& subfiles) const {
	this->previewCompact->populate(path, attrs, subdirs, subfiles);
	this->previewDetails->populate(path, attrs, subdirs, subfiles);
}

void VPreviewDirectoryParent::onDirViewModeUpdated(VPreviewDirectoryParent::ViewMode mode) {
	switch (mode) {
		case VPreviewDirectoryParent::ViewMode::ICONS:
		case VPreviewDirectoryParent::ViewMode::COMPACT:
			this->previewCompact->setCompactOrIcons(mode == VPreviewDirectoryParent::ViewMode::ICONS);
			this->setCurrentWidget(this->previewCompact);
			break;
		case VPreviewDirectoryParent::ViewMode::DETAILS:
			this->setCurrentWidget(this->previewDetails);
			break;
	}
}

void VPreviewDirectoryParent::onProcessedThumbnail(const QString& path, const QIcon& thumbnail) const {
	this->previewCompact->onProcessedThumbnail(path, thumbnail);
	this->previewDetails->onProcessedThumbnail(path, thumbnail);
}
