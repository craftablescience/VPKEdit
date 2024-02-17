#include "EntryOptionsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <vpkedit/format/VPK.h>

#include "../config/Options.h"

using namespace vpkedit;

EntryOptionsDialog::EntryOptionsDialog(bool edit, bool isDir, const QString& prefilledPath,  PackFileType type, EntryOptions options, QWidget* parent)
		: QDialog(parent) {
	const bool advancedFileProps = Options::get<bool>(OPT_ADVANCED_FILE_PROPS);

	this->setModal(true);
	this->setWindowTitle(tr("%1%2 %3").arg(advancedFileProps ? "(Advanced) " : "", edit ? "Edit" : "New", isDir ? "Folder" : "File"));
	// This works well enough without messing around with QFontMetrics
	this->setMinimumWidth(static_cast<int>(130 + (prefilledPath.length() * 8)));

	auto* layout = new QFormLayout(this);

	auto* pathLineEditLabel = new QLabel(tr("The path of the %1:\n(e.g. \"%2\")").arg(isDir ? "folder" : "file", isDir ? "materials/dev" : "materials/cable.vmt"), this);
	this->path = new QLineEdit(this);
	this->path->setText(prefilledPath);
	layout->addRow(pathLineEditLabel, this->path);

	this->useArchiveVPK = nullptr;
	this->preloadBytes = nullptr;
	this->useCompression = nullptr;
	if (advancedFileProps) {
		if (type == PackFileType::VPK) {
			auto* useArchiveVPKLabel = new QLabel(tr("Save %1 file to a new numbered archive\ninstead of the directory VPK:").arg(isDir ? "each" : "the"), this);
			this->useArchiveVPK = new QCheckBox(this);
			this->useArchiveVPK->setCheckState(options.vpk_saveToDirectory ? Qt::CheckState::Unchecked : Qt::CheckState::Checked);
			layout->addRow(useArchiveVPKLabel, this->useArchiveVPK);

			auto* preloadBytesLabel = new QLabel(tr("Set the bytes of %1 file to preload:\n(From 0 to %2 bytes)").arg(isDir ? "each" : "the").arg(VPK_MAX_PRELOAD_BYTES), this);
			this->preloadBytes = new QSpinBox(this);
			this->preloadBytes->setMinimum(0);
			this->preloadBytes->setMaximum(VPK_MAX_PRELOAD_BYTES);
			this->preloadBytes->setValue(static_cast<int>(options.vpk_preloadBytes));
			layout->addRow(preloadBytesLabel, this->preloadBytes);
		}

		if (type == PackFileType::ZIP || type == PackFileType::BSP) {
			auto* useCompressionLabel = new QLabel(tr("Save %1 file with LZMA compression:").arg(isDir ? "each" : "the"), this);
			this->useCompression = new QCheckBox(this);
			this->useCompression->setCheckState(options.zip_compressionMethod == 0 ? Qt::CheckState::Unchecked : Qt::CheckState::Checked);
			layout->addRow(useCompressionLabel, this->useCompression);
		}
	}

	auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	layout->addWidget(buttonBox);

	QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &EntryOptionsDialog::accept);
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &EntryOptionsDialog::reject);
}

EntryOptions EntryOptionsDialog::getEntryOptions() const {
	return {
		.vpk_saveToDirectory = this->useArchiveVPK && !this->useArchiveVPK->isChecked(),
		.vpk_preloadBytes = this->preloadBytes ? static_cast<std::uint32_t>(this->preloadBytes->value()) : 0,
		.zip_compressionMethod = static_cast<uint16_t>(this->useCompression ? this->useCompression->isChecked() ? MZ_COMPRESS_METHOD_LZMA : MZ_COMPRESS_METHOD_STORE : MZ_COMPRESS_METHOD_STORE),
	};
}

std::optional<std::tuple<QString, EntryOptions>> EntryOptionsDialog::getEntryOptions(bool edit, bool isDir, const QString& prefilledPath, vpkedit::PackFileType type, vpkedit::EntryOptions options, QWidget* parent) {
	auto* dialog = new EntryOptionsDialog(edit, isDir, prefilledPath, type, options, parent);
	int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return std::make_tuple(QDir::cleanPath(dialog->path->text().toLower()), dialog->getEntryOptions());
}
