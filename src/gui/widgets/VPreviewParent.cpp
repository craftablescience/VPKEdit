#include "VPreviewParent.hpp"

#include <QFileInfo>
#include <QPlainTextEdit>

#include "plugins/VPluginLoader.hpp"
#include "widgets/previews/VPreviewDirectoryParent.hpp"
#include "widgets/previews/VPreviewInfo.hpp"
#include "widgets/previews/VPreviewNone.hpp"

VPreviewParent::VPreviewParent(QWidget* parent) : QStackedWidget{parent} {
	this->addWidget(this->previewNone = new VPreviewNone{this});
	this->addWidget(this->previewInfo = new VPreviewInfo{this});
	this->addWidget(this->previewDirectory = new VPreviewDirectoryParent{this});

	this->previewPlugins = new VPreviewPluginLoader{this};
	this->addWidget(this->previewPlugins->previewStackedWidget());
	// todo: provide window access
	this->previewPlugins->reloadPlugins(nullptr);

	this->clearPreview();
}

void VPreviewParent::clearPreview() {
	this->setCurrentWidget(this->previewNone);
}

void VPreviewParent::onDirSelected(const QString& path, vpkpp::Attribute attrs, const QStringList& subdirs, const QMap<QString, std::optional<vpkpp::Entry>>& subfiles) {
	this->previewDirectory->populate(path, attrs, subdirs, subfiles);
	this->setCurrentWidget(this->previewDirectory);
}

void VPreviewParent::onFileSelected(const QString& path, const QByteArray& data) {
	if ((path.isEmpty() && data.isEmpty()) || !this->previewPluginLoader()->hasPreview(path)) {
		this->previewInfo->setIcon({":/icons/error_greyscale.png"});
		this->previewInfo->setText(tr("No available preview for this file type."));
		this->setCurrentWidget(this->previewInfo);
		return;
	}
	if (const auto errorMsg = this->previewPluginLoader()->loadPreview(path, reinterpret_cast<const quint8*>(data.data()), data.size())) {
		this->previewInfo->setIcon({":/icons/error.png"});
		this->previewInfo->setText(*errorMsg);
		this->setCurrentWidget(this->previewInfo);
	} else {
		this->setCurrentWidget(this->previewPluginLoader()->previewStackedWidget());
	}
}
