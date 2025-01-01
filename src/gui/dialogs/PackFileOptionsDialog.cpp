#include "PackFileOptionsDialog.h"

#include <bsppp/PakLump.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>
#include <vpkpp/format/FPX.h>
#include <vpkpp/format/VPK.h>
#include <vpkpp/format/ZIP.h>

using namespace vpkpp;

namespace {

int compressionTypeToComboIndex(EntryCompressionType type) {
	switch (type) {
		case EntryCompressionType::NO_OVERRIDE: return 1;
		case EntryCompressionType::NO_COMPRESS: return 0;
		case EntryCompressionType::DEFLATE:     return 3;
		case EntryCompressionType::BZIP2:       return 4;
		case EntryCompressionType::LZMA:        return 2;
		case EntryCompressionType::ZSTD:        return 5;
		case EntryCompressionType::XZ:          return 6;
	}
	return 0;
}

EntryCompressionType comboIndexToCompressionType(int index) {
	switch (index) {
		case 1: return EntryCompressionType::NO_OVERRIDE;
		case 0: return EntryCompressionType::NO_COMPRESS;
		case 3: return EntryCompressionType::DEFLATE;
		case 4: return EntryCompressionType::BZIP2;
		case 2: return EntryCompressionType::LZMA;
		case 5: return EntryCompressionType::ZSTD;
		case 6: return EntryCompressionType::XZ;
	}
	return EntryCompressionType::NO_OVERRIDE;
}

} // namespace

PackFileOptionsDialog::PackFileOptionsDialog(std::string_view typeGUID, bool editing, bool createFromDir, PackFileOptions options, QWidget* parent)
		: QDialog(parent) {
	this->setModal(true);
	this->setWindowTitle(tr("Pack File Properties"));

	auto* layout = new QFormLayout(this);

	this->compressionType = nullptr;
	this->compressionStrength = nullptr;
	if (typeGUID == bsppp::PakLump::GUID || typeGUID == ZIP::GUID) {
		this->compressionType = new QComboBox(this);
		this->compressionType->addItem(tr("None"));
		this->compressionType->addItem(tr("Per-entry"));
		this->compressionType->addItem("LZMA");
		if (typeGUID == ZIP::GUID) {
			this->compressionType->addItem("DEFLATE");
			this->compressionType->addItem("BZIP2");
			this->compressionType->addItem("ZSTD");
			this->compressionType->addItem("XZ");
		}
		this->compressionType->setCurrentIndex(::compressionTypeToComboIndex(options.compressionType));
		layout->addRow(tr("Compression Type Override:"), this->compressionType);

		this->compressionStrength = new QSpinBox(this);
		this->compressionStrength->setMinimum(0);
		this->compressionStrength->setMaximum(9);
		this->compressionStrength->setValue(options.compressionStrength);
		layout->addRow(tr("Compression Strength:"), this->compressionStrength);
	}

	this->version = nullptr;
	if (typeGUID == VPK::GUID) {
		this->version = new QComboBox(this);
		this->version->addItem("v0");
		this->version->addItem("v1");
		this->version->addItem("v2");
		if (options.vpk_version <= 2) {
			this->version->setCurrentIndex(static_cast<int>(options.vpk_version));
		}
		layout->addRow(tr("Version:"), this->version);
	}

	this->singleFile = nullptr;
	this->preferredChunkSize = nullptr;
	if (typeGUID == FPX::GUID || typeGUID == VPK::GUID) {
		if (!editing && createFromDir) {
			this->singleFile = new QCheckBox(this);
			this->singleFile->setChecked(options.vpk_saveSingleFile);
			layout->addRow(tr("Save to single file:\nBreaks if the file's size will be >= 4gb!"), this->singleFile);
		}

		this->preferredChunkSize = new QSpinBox(this);
		this->preferredChunkSize->setMinimum(1); // 1mb
		this->preferredChunkSize->setMaximum(4000); // 4gb
		this->preferredChunkSize->setValue(static_cast<int>(options.vpk_chunkSize / 1024 / 1024));
		layout->addRow(tr("Preferred chunk size (MB):"), this->preferredChunkSize);
	}

	auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	layout->addWidget(buttonBox);

	QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &PackFileOptionsDialog::accept);
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &PackFileOptionsDialog::reject);
}

PackFileOptions PackFileOptionsDialog::getPackFileOptions() const {
	return {
		.compressionType = this->compressionType ? ::comboIndexToCompressionType(this->compressionType->currentIndex()) : EntryCompressionType::NO_OVERRIDE,
		.compressionStrength = static_cast<short>(this->compressionStrength ? this->compressionStrength->value() : 5),
		.vpk_version = this->version ? static_cast<std::uint32_t>(this->version->currentIndex()) : 2,
		.vpk_saveSingleFile = this->singleFile && this->singleFile->isChecked(),
		.vpk_chunkSize = this->preferredChunkSize ? this->preferredChunkSize->value() * 1024 * 1024 : VPK_DEFAULT_CHUNK_SIZE,
	};
}

std::optional<PackFileOptions> PackFileOptionsDialog::getForNew(std::string_view typeGUID, bool createFromDir, QWidget* parent) {
	if (typeGUID != bsppp::PakLump::GUID && typeGUID != FPX::GUID && typeGUID != VPK::GUID && typeGUID != ZIP::GUID) {
		return PackFileOptions{};
	}

	auto* dialog = new PackFileOptionsDialog(typeGUID, false, createFromDir, {}, parent);
	int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return dialog->getPackFileOptions();
}

std::optional<PackFileOptions> PackFileOptionsDialog::getForEdit(std::string_view typeGUID, PackFileOptions options, QWidget* parent) {
	if (typeGUID != bsppp::PakLump::GUID && typeGUID != FPX::GUID && typeGUID != VPK::GUID && typeGUID != ZIP::GUID) {
		QMessageBox::information(parent, tr("Pack File Properties"), tr("No properties available for this file type."));
		return std::nullopt;
	}

	auto* dialog = new PackFileOptionsDialog(typeGUID, false, false, options, parent);
	int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return dialog->getPackFileOptions();
}
