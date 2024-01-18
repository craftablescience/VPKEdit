#include "ControlsDialog.h"

#include <QFile>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

QString getControlsText() {
	QString controlsText;
	QFile controlsFile(":/CONTROLS.md");
	if (controlsFile.open(QIODevice::ReadOnly)) {
		QTextStream in(&controlsFile);
		controlsText = in.readAll();
		controlsFile.close();
	}
	return controlsText;
}

} // namespace

ControlsDialog::ControlsDialog(QWidget* parent)
		: QDialog(parent) {
	this->setModal(true);
	this->setWindowTitle(tr("Controls"));
	this->setFixedSize(600, 600);

	//auto* layout = new QVBoxLayout(this);

	auto* controlsLabel = new QLabel(::getControlsText(), this);
	controlsLabel->setTextFormat(Qt::MarkdownText);

	auto* scroll = new QScrollArea(this);
	scroll->setFixedSize(600, 600);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setWidget(controlsLabel);
}

void ControlsDialog::showControlsDialog(QWidget* parent) {
	auto* dialog = new ControlsDialog(parent);
	dialog->exec();
	dialog->deleteLater();
}
