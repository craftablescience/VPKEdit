#include "EntryOptionsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <vpkedit/format/VPK.h>

#include "../utility/Options.h"

using namespace vpkedit;

EntryOptionsDialog::EntryOptionsDialog(bool edit, bool isDir, const QString& prefilledPath,  PackFileType type, EntryOptions options, QWidget* parent)
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

	this->useArchiveVPK = nullptr;
	this->preloadBytes = nullptr;
	if (advancedFileProps) {
		if (type == PackFileType::VPK) {
			auto* useArchiveVPKLabel = new QLabel(isDir ?
					tr("Save each file to a new numbered archive\ninstead of the directory VPK:") :
					tr("Save the file to a new numbered archive\ninstead of the directory VPK:"), this);
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
		.vpk_saveToDirectory = this->useArchiveVPK && !this->useArchiveVPK->isChecked(),
		.vpk_preloadBytes = this->preloadBytes ? static_cast<std::uint32_t>(this->preloadBytes->value()) : 0,
	};
}

std::optional<std::tuple<QString, EntryOptions>> EntryOptionsDialog::getEntryOptions(bool edit, bool isDir, const QString& prefilledPath, vpkedit::PackFileType type, vpkedit::EntryOptions options, QWidget* parent) {
	auto* dialog = new EntryOptionsDialog(edit, isDir, prefilledPath, type, options, parent);
	int ret = dialog->exec();
	dialog->deleteLater();
	if (ret != QDialog::Accepted) {
		return std::nullopt;
	}
	return std::make_tuple(QDir::cleanPath(dialog->path->text()), dialog->getEntryOptions());
}
