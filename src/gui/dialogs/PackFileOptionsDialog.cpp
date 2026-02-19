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

PackFileOptionsDialog::PackFileOptionsDialog(PackFileOptionsShowForTypeFlags flags, bool editing, bool createFromDir, PackFileOptions options, QWidget* parent)
		: QDialog(parent) {
	this->setModal(true);
	this->setWindowTitle(tr("Pack File Properties"));

	auto* layout = new QFormLayout(this);

	this->compressionType = nullptr;
	this->compressionStrength = nullptr;
	if (static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::BSP) || static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::ZIP)) {
		this->compressionType = new QComboBox(this);
		this->compressionType->addItem(tr("None"), static_cast<int>(EntryCompressionType::NO_COMPRESS));
		this->compressionType->addItem(tr("Per-entry"), static_cast<int>(EntryCompressionType::NO_OVERRIDE));
		this->compressionType->addItem("LZMA", static_cast<int>(EntryCompressionType::LZMA));
		if (static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::ZIP)) {
			this->compressionType->addItem("DEFLATE", static_cast<int>(EntryCompressionType::DEFLATE));
			this->compressionType->addItem("BZIP2", static_cast<int>(EntryCompressionType::BZIP2));
			this->compressionType->addItem("ZSTD", static_cast<int>(EntryCompressionType::ZSTD));
			this->compressionType->addItem("XZ", static_cast<int>(EntryCompressionType::XZ));
		}
		if (const auto index = this->compressionType->findData(static_cast<int>(options.compressionType)); index >= 0) {
			this->compressionType->setCurrentIndex(index);
		}
		layout->addRow(tr("Compression Type Override:"), this->compressionType);

		this->compressionStrength = new QSpinBox(this);
		this->compressionStrength->setMinimum(0);
		this->compressionStrength->setMaximum(9);
		this->compressionStrength->setValue(options.compressionStrength);
		layout->addRow(tr("Compression Strength:"), this->compressionStrength);
	}

	this->version = nullptr;
	if (static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::VPK)) {
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
	if (static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::FPX) || static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::VPK)) {
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
		.compressionType = this->compressionType ? static_cast<EntryCompressionType>(this->compressionType->currentData().toInt()) : EntryCompressionType::NO_OVERRIDE,
		.compressionStrength = static_cast<short>(this->compressionStrength ? this->compressionStrength->value() : 5),
		.vpk_version = this->version ? static_cast<std::uint32_t>(this->version->currentIndex()) : 2,
		.vpk_saveSingleFile = this->singleFile && this->singleFile->isChecked(),
		.vpk_chunkSize = this->preferredChunkSize ? this->preferredChunkSize->value() * 1024 * 1024 : VPK_DEFAULT_CHUNK_SIZE,
	};
}

std::optional<PackFileOptions> PackFileOptionsDialog::getForNew(PackFileOptionsShowForTypeFlags flags, bool createFromDir, QWidget* parent) {
	if (flags == PackFileOptionsShowForTypeFlags::ORDINARY) {
		return PackFileOptions{};
	}

	auto* dialog = new PackFileOptionsDialog(flags, false, createFromDir, {}, parent);
	const int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return dialog->getPackFileOptions();
}

std::optional<PackFileOptions> PackFileOptionsDialog::getForEdit(PackFileOptionsShowForTypeFlags flags, PackFileOptions options, QWidget* parent) {
	if (flags == PackFileOptionsShowForTypeFlags::ORDINARY) {
		QMessageBox::information(parent, tr("Pack File Properties"), tr("No properties available for this file type."));
		return std::nullopt;
	}

	auto* dialog = new PackFileOptionsDialog(flags, false, false, options, parent);
	const int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return dialog->getPackFileOptions();
}
