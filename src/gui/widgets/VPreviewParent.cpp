#include "VPreviewParent.hpp"

#include <QFileInfo>
#include <QPlainTextEdit>

#include "Config.hpp"

#include "plugins/VPluginLoader.hpp"
#include "widgets/previews/VPreviewDirectoryParent.hpp"
#include "widgets/previews/VPreviewInfo.hpp"
#include "widgets/previews/VPreviewNone.hpp"

VPreviewParent::VPreviewParent(QWidget* parent) : QStackedWidget{parent} {
	this->addWidget(this->previewNone = new VPreviewNone{this});
	this->addWidget(this->previewInfo = new VPreviewInfo{this});
	this->addWidget(this->previewDirectory = new VPreviewDirectoryParent{this});

	// todo: handle previews better lol
	this->addWidget(new QPlainTextEdit{this});

	// todo: move somewhere else
	VPluginLoader::iterate(PROJECT_NAME "_previews", [this](const QString& libraryPath) {
		auto* loader = new QPluginLoader{libraryPath, this};
		if (auto* plugin = qobject_cast<VPreviewPlugin_V6_0*>(loader->instance())) {
			if (!loader->metaData().contains("MetaData") || !loader->metaData().value("MetaData").isObject()) {
				return;
			}
			this->previewPluginLoaders.push_back(loader);

			for (const auto& ext : plugin->getFileExtensions()) {
				if (!this->previewPlugins.contains(ext) || this->previewPlugins[ext].second->getPriority() < plugin->getPriority()) {
					this->previewPlugins[ext] = {this->count(), plugin};
				}
			}

			// todo: provide window access
			plugin->initPlugin(nullptr);
			this->addWidget(plugin->getPreview(this));
			// todo: register the plugin somewhere in the ui
			//this->window->registerPlugin(libraryPath, plugin->getIcon(), loader->metaData()["MetaData"].toObject());
		} else {
			loader->deleteLater();
		}
	});

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

	if (const auto ext = QFileInfo{path}.suffix(); this->previewPlugins.contains(ext)) {
		this->setCurrentIndex(this->previewPlugins[ext].first);
		(void) this->previewPlugins[ext].second->setData(this->currentWidget(), path, reinterpret_cast<const quint8*>(data.data()), data.size());
	} else {
		// todo: lol
		this->setCurrentIndex(3);
		qobject_cast<QPlainTextEdit*>(this->currentWidget())->setPlainText(data);
	}
}
