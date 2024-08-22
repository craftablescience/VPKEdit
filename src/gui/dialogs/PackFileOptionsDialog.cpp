#include "PackFileOptionsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>
#include <vpkpp/format/VPK.h>

using namespace vpkpp;

PackFileOptionsDialog::PackFileOptionsDialog(vpkpp::PackFileType type, bool editing, bool createFromDir, PackFileOptions options, QWidget* parent)
		: QDialog(parent) {
	this->setModal(true);
	this->setWindowTitle(tr("Pack File Properties"));

	auto* layout = new QFormLayout(this);

	this->version = new QComboBox(this);
	this->version->addItem("v1");
	this->version->addItem("v2");
	this->version->setCurrentIndex(static_cast<int>(options.vpk_version) - 1);
	layout->addRow(tr("Version:"), this->version);

	this->singleFile = nullptr;
	if (!editing && createFromDir) {
		this->singleFile = new QCheckBox(this);
		this->singleFile->setChecked(options.vpk_saveSingleFile);
		layout->addRow(tr("Save to single file:\nBreaks the VPK if its size will be >= 4gb!"), this->singleFile);
	}

	this->preferredChunkSize = new QSpinBox(this);
	this->preferredChunkSize->setMinimum(1); // 1mb
	this->preferredChunkSize->setMaximum(4000); // 4gb
	this->preferredChunkSize->setValue(static_cast<int>(options.vpk_chunkSize / 1024 / 1024));
	layout->addRow(tr("Preferred chunk size (MB):"), this->preferredChunkSize);

	auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	layout->addWidget(buttonBox);

	QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &PackFileOptionsDialog::accept);
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &PackFileOptionsDialog::reject);
}

PackFileOptions PackFileOptionsDialog::getPackFileOptions() const {
	return {
		.vpk_version = static_cast<std::uint32_t>(this->version->currentIndex() + 1), // VPK v1, v2
		.vpk_saveSingleFile = this->singleFile && this->singleFile->isChecked(),
		.vpk_chunkSize = this->preferredChunkSize ? this->preferredChunkSize->value() * 1024 * 1024 : VPK_DEFAULT_CHUNK_SIZE,
	};
}

std::optional<PackFileOptions> PackFileOptionsDialog::getForNew(vpkpp::PackFileType type, bool createFromDir, QWidget* parent) {
	if (type != PackFileType::VPK) {
		return PackFileOptions{};
	}

	auto* dialog = new PackFileOptionsDialog(type, false, createFromDir, {}, parent);
	int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return dialog->getPackFileOptions();
}

std::optional<PackFileOptions> PackFileOptionsDialog::getForEdit(vpkpp::PackFileType type, PackFileOptions options, QWidget* parent) {
	if (type != PackFileType::VPK) {
		QMessageBox::information(parent, tr("Pack File Properties"), tr("No properties available for this file type."));
		return std::nullopt;
	}

	auto* dialog = new PackFileOptionsDialog(type, false, false, options, parent);
	int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return dialog->getPackFileOptions();
}
