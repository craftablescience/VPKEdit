#include "VPreviewParent.hpp"

#include <QPlainTextEdit>

#include "previews/VPreviewDirectoryParent.hpp"
#include "previews/VPreviewInfo.hpp"
#include "previews/VPreviewNone.hpp"

VPreviewParent::VPreviewParent(QWidget* parent) : QStackedWidget{parent} {
	this->addWidget(this->previewNone = new VPreviewNone{this});
	this->addWidget(this->previewInfo = new VPreviewInfo{this});
	this->addWidget(this->previewDirectory = new VPreviewDirectoryParent{this});

	// todo: handle previews better lol
	this->addWidget(new QPlainTextEdit{this});

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
	if (path.isEmpty() && data.isEmpty()) {
		this->clearPreview();
		return;
	}

	// todo: lol
	this->setCurrentIndex(3);
	qobject_cast<QPlainTextEdit*>(this->currentWidget())->setPlainText(data);
}
