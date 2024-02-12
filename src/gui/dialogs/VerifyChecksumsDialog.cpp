#include "VerifyChecksumsDialog.h"

#include <vpkedit/PackFile.h>

using namespace vpkedit;

VerifyChecksumsDialog::VerifyChecksumsDialog(PackFile& packFile, QWidget* parent)
		: QMessageBox(parent) {
	this->setModal(true);
	this->setWindowTitle(tr("Verify Checksums"));

	QString text;
	if (packFile.verifyFileChecksum()) {
		text = u8"✅ " + tr("Overall file checksum matches the expected value.");
	} else {
		text = u8"❌ " + tr("Overall file checksum does not match the expected value!");
	}
	text += "\n\n";

	if (auto entries = packFile.verifyEntryChecksums(); entries.empty()) {
		text += u8"✅ " + tr("All file checksums match their expected values.");
	} else {
		text += u8"❌ " + tr("Some file checksums do not match their expected values!\nSee below for more information.");

		QString details = tr("Files that failed to validate:\n");
		for (const auto& entryPath : entries) {
			details += entryPath.c_str();
			details += '\n';
		}
		this->setDetailedText(details);
	}
	this->setText(text);

	this->setStandardButtons(QMessageBox::StandardButton::Close);
}

void VerifyChecksumsDialog::showDialog(PackFile& packFile, QWidget* parent) {
	auto* dialog = new VerifyChecksumsDialog(packFile, parent);
	dialog->exec();
	dialog->deleteLater();
}
