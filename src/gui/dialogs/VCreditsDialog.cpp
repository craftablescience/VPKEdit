#include "VCreditsDialog.hpp"

#include <ranges>

#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

#include "Config.hpp"

namespace {

[[nodiscard]] QString readCreditsFile(const QString& name) {
	QString creditsText;
	QFile creditsFile(QString{":/attribution/%1"}.arg(name));
	if (creditsFile.open(QIODevice::ReadOnly)) {
		QTextStream in(&creditsFile);
		creditsText = in.readAll();
		creditsFile.close();
	}
	return creditsText;
}

[[nodiscard]] QList<QPair<QString, QString>> getCreditsTexts() {
	QList<QPair<QString, QString>> creditsTextSplit;
	for (const auto& str : ::readCreditsFile("CREDITS").split('\n')) {
		if (str.startsWith("##")) {
			creditsTextSplit.append({str.sliced(2).trimmed(), {}});
		} else if (!creditsTextSplit.isEmpty()) {
			creditsTextSplit.back().second += str + '\n';
		}
	}
	return creditsTextSplit;
}

[[nodiscard]] QList<QPair<QString, QString>> getLicenseTexts() {
	QList<QPair<QString, QString>> licenseTextSplit;
	for (std::string_view pathView : {"argparse", "RapidJSON", "Discord RPC", "SourcePP"}) {
		licenseTextSplit.append({pathView.data(), ::readCreditsFile(QString{"LICENSE_%1"}.arg(pathView.data()).toUpper().replace(' ', '_'))});
	}
	for (auto& str : ::readCreditsFile("LICENSE_SOURCEPP_THIRDPARTY").split('\n')) {
		if (str.startsWith("----------")) {
			licenseTextSplit.append({str.remove(QRegularExpression{"[\\-]{2,}"}).trimmed(), {}});
		} else if (!licenseTextSplit.empty()) {
			licenseTextSplit.back().second += str + '\n';
		}
	}
	std::ranges::sort(licenseTextSplit, [](const auto& lhs, const auto& rhs) { return lhs.first.toLower() < rhs.first.toLower(); });
	for (auto& licenseText : licenseTextSplit | std::views::values) {
		licenseText = licenseText.trimmed();
	}
	return licenseTextSplit;
}

} // namespace

QSize VLicenseTabStyle::sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const {
	QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
	if (type == QStyle::CT_TabBarTab) {
		s.transpose();
	}
	return s;
}

void VLicenseTabStyle::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
	if (element == CE_TabBarTabLabel) {
		if (const auto* tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
			QStyleOptionTab opt{*tab};
			opt.shape = QTabBar::RoundedNorth;
			QProxyStyle::drawControl(element, &opt, painter, widget);
			return;
		}
	}
	QProxyStyle::drawControl(element, option, painter, widget);
}

VCreditsDialog::VCreditsDialog(QWidget* parent) : QDialog{parent} {
	this->setWindowTitle(tr("Credits"));
	this->setModal(true);
	this->setMinimumSize(600, 600);

	// --- Widgets ---

	auto* layout = new QVBoxLayout{this};

	auto* header = new QWidget{this};
	layout->addWidget(header);

	auto* headerLayout = new QHBoxLayout{header};

	auto* projectLogo = new QLabel{this};
	projectLogo->setPixmap(QPixmap{":/logo.png"}.scaledToHeight(90));
	headerLayout->addWidget(projectLogo);

	headerLayout->addSpacing(16);

	auto* creditsLabel = new QLabel{QString{"## %1\n*Created by [craftablescience](https://github.com/craftablescience)*\n<br/>\n"}.arg(PROJECT_TITLE), this};
	creditsLabel->setTextFormat(Qt::MarkdownText);
	creditsLabel->setOpenExternalLinks(true);
	creditsLabel->setAlignment(Qt::AlignBottom);
	headerLayout->addWidget(creditsLabel, Qt::AlignLeft);

	auto* tabs = new QTabWidget{this};

	for (const auto& [name, content] : ::getCreditsTexts()) {
		auto* tabContent = new QLabel{content, tabs};
		tabContent->setTextFormat(Qt::MarkdownText);
		tabContent->setOpenExternalLinks(true);
		auto* tabScroll = new QScrollArea{this};
		tabScroll->setWidget(tabContent);
		tabs->addTab(tabScroll, name);
	}

	auto* licenseTabs = new QTabWidget{tabs};
	licenseTabs->setDocumentMode(true);
	licenseTabs->setTabPosition(QTabWidget::TabPosition::West);
	licenseTabs->tabBar()->setStyle(new VLicenseTabStyle);
	for (const auto& [name, content] : ::getLicenseTexts()) {
		auto* tabContent = new QLabel{content, licenseTabs};
		auto* tabScroll = new QScrollArea{this};
		tabScroll->setWidget(tabContent);
		licenseTabs->addTab(tabScroll, name);
	}
	tabs->addTab(licenseTabs, "Licenses");

	layout->addWidget(tabs);

	auto* buttons = new QDialogButtonBox{QDialogButtonBox::Ok, this};

	auto* buttonAboutQt = buttons->addButton(tr("About Qt"), QDialogButtonBox::NoRole);
	buttonAboutQt->setIcon(this->style()->standardIcon(QStyle::SP_TitleBarMenuButton));

	layout->addWidget(buttons);

	// --- Logic ---

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::rejected);

	connect(buttonAboutQt, &QPushButton::clicked, this, [this] {
		QMessageBox::aboutQt(this);
	});

	// --- Finalize ---

	this->resize(600, 600);
}

void VCreditsDialog::showCredits(QWidget* parent) {
	auto* dialog = new VCreditsDialog{parent};
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->open();
}
