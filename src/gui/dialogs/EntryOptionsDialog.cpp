#include "EntryOptionsDialog.h"

#include <bsppp/PakLump.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <vpkpp/format/FPX.h>
#include <vpkpp/format/VPK.h>
#include <vpkpp/format/ZIP.h>

#include "../utility/Options.h"

using namespace vpkpp;

namespace {

int compressionTypeToComboIndex(EntryCompressionType type) {
	switch (type) {
		case EntryCompressionType::NO_COMPRESS: return 0;
		case EntryCompressionType::DEFLATE:     return 2;
		case EntryCompressionType::BZIP2:       return 3;
		case EntryCompressionType::LZMA:        return 1;
		case EntryCompressionType::ZSTD:        return 4;
		case EntryCompressionType::XZ:          return 5;
		default: break;
	}
	return 0;
}

EntryCompressionType comboIndexToCompressionType(int index) {
	switch (index) {
		case 0: return EntryCompressionType::NO_COMPRESS;
		case 2: return EntryCompressionType::DEFLATE;
		case 3: return EntryCompressionType::BZIP2;
		case 1: return EntryCompressionType::LZMA;
		case 4: return EntryCompressionType::ZSTD;
		case 5: return EntryCompressionType::XZ;
		default: break;
	}
	return EntryCompressionType::NO_COMPRESS;
}

} // namespace

EntryOptionsDialog::EntryOptionsDialog(bool edit, bool isDir, const QString& prefilledPath, PackFileOptionsShowForTypeFlags flags, EntryOptions options, QWidget* parent)
		: QDialog(parent) {
	const bool advancedFileProps = Options::get<bool>(OPT_ADVANCED_FILE_PROPS);

	this->setModal(true);

	// For the sake of proper translations
	QString title;
	if (advancedFileProps) {
		if (edit) {
			if (isDir) {
				title = tr("(Advanced) Edit Folder");
			} else {
				title = tr("(Advanced) Edit File");
			}
		} else {
			if (isDir) {
				title = tr("(Advanced) New Folder");
			} else {
				title = tr("(Advanced) New File");
			}
		}
	} else {
		if (edit) {
			if (isDir) {
				title = tr("Edit Folder");
			} else {
				title = tr("Edit File");
			}
		} else {
			if (isDir) {
				title = tr("New Folder");
			} else {
				title = tr("New File");
			}
		}
	}
	this->setWindowTitle(title);

	// This works well enough without messing around with QFontMetrics
	this->setMinimumWidth(static_cast<int>(130 + (prefilledPath.length() * 8)));

	auto* layout = new QFormLayout(this);

	auto* pathLineEditLabel = new QLabel(isDir ?
			tr("The path of the folder:\n(e.g. \"%1\")").arg("materials/dev") :
			tr("The path of the folder:\n(e.g. \"%1\")").arg("materials/cable.vmt"), this);
	this->path = new QLineEdit(this);
	this->path->setText(prefilledPath);
	layout->addRow(pathLineEditLabel, this->path);

	this->compressionType = nullptr;
	this->compressionStrength = nullptr;
	this->useArchiveVPK = nullptr;
	this->preloadBytes = nullptr;
	if (advancedFileProps) {
		if (static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::BSP) || static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::ZIP)) {
			this->compressionType = new QComboBox(this);
			this->compressionType->addItem(tr("None"));
			this->compressionType->addItem("LZMA");
			if (static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::ZIP)) {
				this->compressionType->addItem("DEFLATE");
				this->compressionType->addItem("BZIP2");
				this->compressionType->addItem("ZSTD");
				this->compressionType->addItem("XZ");
			}
			layout->addRow(tr("Compression Type:"), this->compressionType);

			this->compressionStrength = new QSpinBox(this);
			this->compressionStrength->setMinimum(0);
			this->compressionStrength->setMaximum(9);
			this->compressionStrength->setValue(options.zip_compressionStrength);
			layout->addRow(tr("Compression Strength Override:"), this->compressionStrength);
		} else if (static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::FPX) || static_cast<bool>(flags & PackFileOptionsShowForTypeFlags::VPK)) {
			auto* useArchiveVPKLabel = new QLabel(isDir ?
					tr("Save each file to a new numbered archive\ninstead of the directory:") :
					tr("Save the file to a new numbered archive\ninstead of the directory:"), this);
			this->useArchiveVPK = new QCheckBox(this);
			this->useArchiveVPK->setCheckState(options.vpk_saveToDirectory ? Qt::CheckState::Unchecked : Qt::CheckState::Checked);
			layout->addRow(useArchiveVPKLabel, this->useArchiveVPK);

			auto* preloadBytesLabel = new QLabel(isDir ?
					tr("Set the bytes of each file to preload:\n(From 0 to %1 bytes)").arg(VPK_MAX_PRELOAD_BYTES) :
					tr("Set the bytes of the file to preload:\n(From 0 to %1 bytes)").arg(VPK_MAX_PRELOAD_BYTES), this);
			this->preloadBytes = new QSpinBox(this);
			this->preloadBytes->setMinimum(0);
			this->preloadBytes->setMaximum(VPK_MAX_PRELOAD_BYTES);
			this->preloadBytes->setValue(static_cast<int>(options.vpk_preloadBytes));
			layout->addRow(preloadBytesLabel, this->preloadBytes);
		}
	}

	auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	layout->addWidget(buttonBox);

	QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &EntryOptionsDialog::accept);
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &EntryOptionsDialog::reject);
}

EntryOptions EntryOptionsDialog::getEntryOptions() const {
	return {
		.zip_compressionType = this->compressionType ? ::comboIndexToCompressionType(this->compressionType->currentIndex()) : EntryCompressionType::NO_COMPRESS,
		.zip_compressionStrength = static_cast<int16_t>(this->compressionStrength ? this->compressionStrength->value() : 5),
		.vpk_preloadBytes = static_cast<uint16_t>(this->preloadBytes ? this->preloadBytes->value() : 0),
		.vpk_saveToDirectory = this->useArchiveVPK && !this->useArchiveVPK->isChecked(),
	};
}

std::optional<std::tuple<QString, EntryOptions>> EntryOptionsDialog::getEntryOptions(bool edit, bool isDir, const QString& prefilledPath, PackFileOptionsShowForTypeFlags flags, EntryOptions options, QWidget* parent) {
	auto* dialog = new EntryOptionsDialog(edit, isDir, prefilledPath, flags, options, parent);
	const int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return std::make_tuple(QDir::cleanPath(dialog->path->text()), dialog->getEntryOptions());
}
