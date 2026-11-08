#include "VPluginDialog.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include "utility/VHorizontalTabStyle.hpp"

VPluginDialog::VPluginDialog(const QMap<QString, QList<VPluginMetadata>>& pluginMetadata, QWidget* parent) : QDialog{parent} {
	this->setWindowTitle(tr("Plugins"));
	this->setModal(true);
	this->setMinimumSize(600, 600);

	// --- Widgets ---

	auto* layout = new QVBoxLayout{this};

	auto* tabs = new QTabWidget{this};

	for (const auto& [pluginCategory, pluginList] : pluginMetadata.asKeyValueRange()) {
		auto* pluginTabs = new QTabWidget{tabs};
		pluginTabs->setDocumentMode(true);
		pluginTabs->setTabPosition(QTabWidget::TabPosition::West);
		pluginTabs->tabBar()->setStyle(new VHorizontalTabStyle);
		// ReSharper disable once CppUseStructuredBinding
		for (const auto& metadata : pluginList) {
			auto* tabContent = new QLabel{pluginTabs};
			tabContent->setAlignment(Qt::AlignTop);
			tabContent->setWordWrap(true);
			tabContent->setTextFormat(Qt::MarkdownText);
			tabContent->setOpenExternalLinks(true);

			auto text = QString{"\n**%1:** %2\n\n**%3:** %4\n\n**%5:** %6\n\n**%7:**\n"}.arg(
				tr("Author"),
				metadata.author,
				tr("Version"),
				metadata.version,
				tr("Description"),
				metadata.description,
				tr("Links")
			);
			for (const auto& [linkName, linkURL] : metadata.links.asKeyValueRange()) {
				text += QString{"\n- **%1:** %2"}.arg(linkName, linkURL);
			}
			tabContent->setText(text);

			auto* tabScroll = new QScrollArea{pluginTabs};
			tabScroll->setWidget(tabContent);
			tabScroll->setWidgetResizable(true);
			pluginTabs->addTab(tabScroll, metadata.name);
		}
		tabs->addTab(pluginTabs, pluginCategory);
	}

	layout->addWidget(tabs);

	auto* buttons = new QDialogButtonBox{QDialogButtonBox::Ok, this};
	layout->addWidget(buttons);

	// --- Logic ---

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::rejected);

	// --- Finalize ---

	this->resize(600, 600);
}

void VPluginDialog::showPlugins(const QMap<QString, QList<VPluginMetadata>>& pluginMetadata, QWidget* parent) {
	auto* dialog = new VPluginDialog{pluginMetadata, parent};
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->open();
}
