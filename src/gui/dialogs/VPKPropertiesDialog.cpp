#include "VPKPropertiesDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>

#include <vpkedit/VPK.h>

using namespace vpkedit;

VPKPropertiesDialog::VPKPropertiesDialog(bool exists, std::uint32_t startVersion, bool singleFile, QWidget* parent)
        : QDialog(parent) {
    this->setModal(true);
    this->setWindowTitle(exists ? tr("Set VPK Version") : tr("New VPK Properties"));

    auto* layout = new QFormLayout(this);

    auto* versionLabel = new QLabel(tr("Version:"), this);
    this->version = new QComboBox(this);
    this->version->addItem(tr("v1"));
    this->version->addItem(tr("v2"));
    this->version->setCurrentIndex(static_cast<int>(startVersion) - 1);
    layout->addRow(versionLabel, this->version);

	if (!exists) {
		auto* singleFileLabel = new QLabel(tr("Save to single file:\nNote: this may not work for VPKs larger than ~2gb!"), this);
		this->singleFile = new QCheckBox(this);
		this->singleFile->setChecked(singleFile);
		layout->addRow(singleFileLabel, this->singleFile);
	} else {
		this->singleFile = nullptr;
	}

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &VPKPropertiesDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &VPKPropertiesDialog::reject);
}

std::optional<std::tuple<std::uint32_t, bool>> VPKPropertiesDialog::getVPKProperties(bool exists, std::uint32_t startVersion, bool singleFile, QWidget* parent) {
    auto* dialog = new VPKPropertiesDialog(exists, startVersion, singleFile, parent);
    int ret = dialog->exec();
    if (ret != QDialog::Accepted) {
        dialog->deleteLater();
        return std::nullopt;
    }
    // v1 - 1, v2 - 2
    return std::make_tuple(dialog->version->currentIndex() + 1, dialog->singleFile && dialog->singleFile->isChecked());
}
