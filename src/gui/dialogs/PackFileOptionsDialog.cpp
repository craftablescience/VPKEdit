#include "PackFileOptionsDialog.h"

#include <QCheckBox>
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
#ifdef VPKEDIT_ZIP_COMPRESSION
	this->zip_useCompression = nullptr;
#endif
	if (type == PackFileType::VPK) {
		auto* versionLabel = new QLabel(tr("Version:"), this);
		this->vpk_version = new QComboBox(this);
		this->vpk_version->addItem("v1");
		this->vpk_version->addItem("v2");
		this->vpk_version->setCurrentIndex(static_cast<int>(this->options.vpk_version) - 1);
		layout->addRow(versionLabel, this->vpk_version);
	}
#ifdef VPKEDIT_ZIP_COMPRESSION
	else if (type == PackFileType::BSP || type == PackFileType::ZIP) {
		auto* useCompressionLabel = new QLabel(tr("Use LZMA Compression:"), this);
		this->zip_useCompression = new QCheckBox(this);
		this->zip_useCompression->setChecked(this->options.zip_compressionMethod == MZ_COMPRESS_METHOD_LZMA);
		layout->addRow(useCompressionLabel, this->zip_useCompression);
	}
#endif
	else {
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
	this->options.zip_compressionMethod =
#ifdef VPKEDIT_ZIP_COMPRESSION
			this->zip_useCompression ? MZ_COMPRESS_METHOD_LZMA :
#endif
			MZ_COMPRESS_METHOD_STORE;
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
