#include "CreditsDialog.h"

#include <QFile>
#include <QLabel>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

#include <Config.h>
#include <QDialogButtonBox>

namespace {

[[nodiscard]] QMap<QString, QString> getCreditsTexts() {
	QString creditsText;
	QFile creditsFile(":/CREDITS.md");
	if (creditsFile.open(QIODevice::ReadOnly)) {
		QTextStream in(&creditsFile);
		creditsText = in.readAll();
		creditsFile.close();
	}
	QMap<QString, QString> creditsTextSplit;
	QString activeKey;
	for (const auto& str : creditsText.split('\n')) {
		if (str.startsWith("##")) {
			activeKey = str.sliced(2).trimmed();
			creditsTextSplit[activeKey] = "";
		} else if (!activeKey.isEmpty()) {
			creditsTextSplit[activeKey] += str + '\n';
		}
	}
	return creditsTextSplit;
}

} // namespace

CreditsDialog::CreditsDialog(QWidget* parent)
		: QDialog(parent) {
	this->setModal(true);
	this->setWindowTitle(tr("About %1").arg(PROJECT_NAME_PRETTY.data()));
	this->setFixedSize(600, 600);

	auto* layout = new QVBoxLayout(this);

	auto* header = new QWidget(this);
	layout->addWidget(header);

	auto* headerLayout = new QHBoxLayout(header);

	auto* projectLogo = new QLabel{this};
	projectLogo->setPixmap(QPixmap{":/logo.png"}.scaledToHeight(90));
	headerLayout->addWidget(projectLogo);

	headerLayout->addSpacing(16);

	auto* creditsLabel = new QLabel(QString("## %1\n*Created by [craftablescience](https://github.com/craftablescience)*\n<br/>\n").arg(PROJECT_TITLE.data()), this);
	creditsLabel->setTextFormat(Qt::MarkdownText);
	creditsLabel->setAlignment(Qt::AlignBottom);
	headerLayout->addWidget(creditsLabel, Qt::AlignLeft);

	auto* tabs = new QTabWidget(this);
	for (const auto& [name, content] : ::getCreditsTexts().asKeyValueRange()) {
		auto* tabContent = new QLabel(content, tabs);
		tabContent->setTextFormat(Qt::MarkdownText);
		auto* tabScroll = new QScrollArea(this);
		tabScroll->setWidget(tabContent);
		tabs->addTab(tabScroll, name);
	}
	layout->addWidget(tabs);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
	QObject::connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	layout->addWidget(buttons);
}

void CreditsDialog::showDialog(QWidget* parent) {
	auto* dialog = new CreditsDialog(parent);
	dialog->exec();
	dialog->deleteLater();
}
