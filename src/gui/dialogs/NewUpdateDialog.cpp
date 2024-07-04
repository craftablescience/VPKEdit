#include "NewUpdateDialog.h"

#include <QGridLayout>
#include <QSpacerItem>

#include <Version.h>

#include "../utility/Options.h"

NewUpdateDialog::NewUpdateDialog(const QString& releaseLink, const QString& version, const QString& details, QWidget* parent)
		: QMessageBox(parent) {
	this->setWindowTitle(tr("New Update Available"));

	this->setText(tr("There is a new update available!\n\n"
					 "Current version:  v%1\n\n"
					 "Latest version:  %2\n\n"
					 "[Click here to go to the release on GitHub.](%3)").arg(PROJECT_VERSION_PRETTY.data(), version, releaseLink));
	this->setTextFormat(Qt::MarkdownText);
	this->setDetailedText(details);

	if (auto* gridLayout = dynamic_cast<QGridLayout*>(this->layout())) {
		auto* horizontalSpacer = new QSpacerItem(300, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
		gridLayout->addItem(horizontalSpacer, gridLayout->rowCount(), 0, 1, gridLayout->columnCount());
	}

	this->setStandardButtons(QMessageBox::StandardButton::Ignore | QMessageBox::StandardButton::Close);
}

void NewUpdateDialog::getNewUpdatePrompt(const QString& releaseLink, const QString& version, const QString& details, QWidget* parent) {
	if (::Options::get<QString>(STR_IGNORED_UPDATE_VERSION) == version) {
		return;
	}

	auto* dialog = new NewUpdateDialog(releaseLink, version, details, parent);
	auto button = dialog->exec();
	if (button == QMessageBox::StandardButton::Ignore) {
		::Options::set(STR_IGNORED_UPDATE_VERSION, version);
	}
	dialog->deleteLater();
}
