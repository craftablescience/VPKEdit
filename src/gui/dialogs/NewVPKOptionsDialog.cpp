#include "NewVPKOptionsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <vpkedit/format/VPK.h>

#include "../utility/Options.h"

using namespace vpkedit;

NewVPKOptionsDialog::NewVPKOptionsDialog(bool fromDirectory, PackFileOptions options, bool singleFile, QWidget* parent)
        : QDialog(parent) {
	const bool advancedFileProps = Options::get<bool>(OPT_ADVANCED_FILE_PROPS);

    this->setModal(true);
    this->setWindowTitle(tr("New VPK Properties"));

    auto* layout = new QFormLayout(this);

    auto* versionLabel = new QLabel(tr("Version:"), this);
    this->version = new QComboBox(this);
    this->version->addItem("v1");
    this->version->addItem("v2");
    this->version->setCurrentIndex(static_cast<int>(options.vpk_version) - 1);
    layout->addRow(versionLabel, this->version);

	this->singleFile = nullptr;
	if (fromDirectory) {
		auto* singleFileLabel = new QLabel(tr("Save to single file:\nBreaks the VPK if its size will be >= 4gb!"), this);
		this->singleFile = new QCheckBox(this);
		this->singleFile->setChecked(singleFile);
		layout->addRow(singleFileLabel, this->singleFile);
	}

	this->preferredChunkSize = nullptr;
	this->generateMD5Entries = nullptr;
	if (advancedFileProps) {
		auto* preferredChunkSizeLabel = new QLabel(tr("Preferred chunk size (MB):"), this);
		this->preferredChunkSize = new QSpinBox(this);
		this->preferredChunkSize->setMinimum(1); // 1mb
		this->preferredChunkSize->setMaximum(4000); // 4gb
		this->preferredChunkSize->setValue(static_cast<int>(options.vpk_preferredChunkSize / 1024 / 1024));
		layout->addRow(preferredChunkSizeLabel, this->preferredChunkSize);

		auto* generateMD5EntriesLabel = new QLabel(tr("Generate per-file MD5 entries:"), this);
		this->generateMD5Entries = new QCheckBox(this);
		this->generateMD5Entries->setChecked(options.vpk_generateMD5Entries);
		layout->addRow(generateMD5EntriesLabel, this->generateMD5Entries);
		if (options.vpk_version == 1) {
			this->generateMD5Entries->setDisabled(true);
		}
		QObject::connect(this->version, &QComboBox::currentIndexChanged, this, [this](int index) {
			this->generateMD5Entries->setDisabled(index == 0);
		});
	}

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &NewVPKOptionsDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &NewVPKOptionsDialog::reject);
}

PackFileOptions NewVPKOptionsDialog::getPackFileOptions() const {
	return {
		.vpk_version = static_cast<std::uint32_t>(this->version->currentIndex() + 1), // VPK v1, v2
		.vpk_preferredChunkSize = this->preferredChunkSize ? this->preferredChunkSize->value() * 1024 * 1024 : VPK_DEFAULT_CHUNK_SIZE,
		.vpk_generateMD5Entries = this->generateMD5Entries && this->version->currentIndex() > 0 && this->generateMD5Entries->isChecked(),
	};
}

std::optional<std::tuple<PackFileOptions, bool>> NewVPKOptionsDialog::getNewVPKOptions(bool fromDirectory, PackFileOptions options, bool singleFile, QWidget* parent) {
    auto* dialog = new NewVPKOptionsDialog(fromDirectory, options, singleFile, parent);
    int ret = dialog->exec();
	dialog->deleteLater();
    if (ret != QDialog::Accepted) {
        return std::nullopt;
    }
    return std::make_tuple(dialog->getPackFileOptions(), dialog->singleFile && dialog->singleFile->isChecked());
}
