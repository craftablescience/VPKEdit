#include "VerifySignatureDialog.h"

#include <vpkpp/vpkpp.h>

using namespace vpkpp;

VerifySignatureDialog::VerifySignatureDialog(PackFile& packFile, QWidget* parent)
		: QMessageBox(parent) {
	this->setModal(true);
	this->setWindowTitle(tr("Verify Signature"));

	if (!packFile.hasPackFileSignature()) {
		this->setText(tr("File does not have a signature."));
	} else if (packFile.verifyPackFileChecksum()) {
		this->setText(u8"✅ " + tr("File signature is valid."));
	} else {
		this->setText(u8"❌ " + tr("File signature is invalid!"));
	}

	this->setStandardButtons(QMessageBox::StandardButton::Close);
}

void VerifySignatureDialog::showDialog(PackFile& packFile, QWidget* parent) {
	auto* dialog = new VerifySignatureDialog(packFile, parent);
	dialog->exec();
	dialog->deleteLater();
}
