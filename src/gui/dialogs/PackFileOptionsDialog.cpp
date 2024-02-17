#include "PackFileOptionsDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <vpkedit/format/VPK.h>

using namespace vpkedit;

PackFileOptionsDialog::PackFileOptionsDialog(PackFileType type, PackFileOptions options_, QWidget* parent)
        : QDialog(parent)
		, options(options_) {
    this->setModal(true);
    this->setWindowTitle(tr("Properties"));

    auto* layout = new QFormLayout(this);

	this->vpk_version = nullptr;
	if (type == PackFileType::VPK) {
		auto* versionLabel = new QLabel(tr("Version:"), this);
		this->vpk_version = new QComboBox(this);
		this->vpk_version->addItem(tr("v1"));
		this->vpk_version->addItem(tr("v2"));
		this->vpk_version->setCurrentIndex(static_cast<int>(this->options.vpk_version) - 1);
		layout->addRow(versionLabel, this->vpk_version);
	}

	if (type == PackFileType::ZIP || type == PackFileType::BSP) {
		auto* nothingLabel = new QLabel(tr("There are no properties available for this file type."), this);
		layout->addWidget(nothingLabel);
	}

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &PackFileOptionsDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &PackFileOptionsDialog::reject);
}

PackFileOptions PackFileOptionsDialog::getPackFileOptions() {
	this->options.vpk_version = this->vpk_version ? this->vpk_version->currentIndex() + 1 : 2;
	return this->options;
}

std::optional<PackFileOptions> PackFileOptionsDialog::getPackFileOptions(PackFileType type, PackFileOptions options, QWidget* parent) {
    auto* dialog = new PackFileOptionsDialog(type, options, parent);
    int ret = dialog->exec();
	dialog->deleteLater();
    if (ret != QDialog::Accepted) {
        return std::nullopt;
    }
    return dialog->getPackFileOptions();
}
