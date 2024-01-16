#include "EntryOptionsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

#include <vpkedit/VPK.h>

#include "../config/Options.h"

using namespace vpkedit;

EntryOptionsDialog::EntryOptionsDialog(bool edit, bool isDir, const QString& prefilledPath, bool prefilledUseArchiveVPK, int prefilledPreloadBytes, QWidget* parent)
        : QDialog(parent) {
    const bool advancedFileProps = Options::get<bool>(OPT_ADVANCED_FILE_PROPS);

    this->setModal(true);
    this->setWindowTitle(tr("%1%2 %3").arg(advancedFileProps ? "(Advanced) " : "", edit ? "Edit" : "New", isDir ? "Folder" : "File"));
	// This works well enough without messing around with QFontMetrics
	this->setMinimumWidth(static_cast<int>(130 + (prefilledPath.length() * 8)));

    auto* layout = new QFormLayout(this);

    auto* pathLineEditLabel = new QLabel(tr("The path of the %1 in the VPK:\n(e.g. \"%2\")").arg(isDir ? "folder" : "file", isDir ? "materials/dev" : "materials/cable.vmt"), this);
    this->path = new QLineEdit(this);
    this->path->setText(prefilledPath);
    layout->addRow(pathLineEditLabel, this->path);

    if (advancedFileProps) {
        auto* useArchiveVPKLabel = new QLabel(tr("Save %1 file to a new numbered archive\ninstead of the directory VPK:").arg(isDir ? "each" : "the"), this);
        this->useArchiveVPK = new QCheckBox(this);
        this->useArchiveVPK->setCheckState(prefilledUseArchiveVPK ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
        layout->addRow(useArchiveVPKLabel, this->useArchiveVPK);

        auto* preloadBytesLabel = new QLabel(tr("Set the bytes of %1 file to preload:\n(From 0 to %2 bytes)").arg(isDir ? "each" : "the").arg(VPK_MAX_PRELOAD_BYTES), this);
        this->preloadBytes = new QSpinBox(this);
        this->preloadBytes->setMinimum(0);
        this->preloadBytes->setMaximum(VPK_MAX_PRELOAD_BYTES);
        this->preloadBytes->setValue(prefilledPreloadBytes);
        layout->addRow(preloadBytesLabel, this->preloadBytes);
    } else {
        this->useArchiveVPK = nullptr;
        this->preloadBytes = nullptr;
    }

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &EntryOptionsDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &EntryOptionsDialog::reject);
}

std::optional<std::tuple<QString, bool, int>> EntryOptionsDialog::getEntryOptions(bool edit, bool isDir, const QString& prefilledPath, bool prefilledUseArchiveVPK, int prefilledPreloadBytes, QWidget* parent) {
    auto* dialog = new EntryOptionsDialog(edit, isDir, prefilledPath, prefilledUseArchiveVPK, prefilledPreloadBytes, parent);
    int ret = dialog->exec();
    if (ret != QDialog::Accepted) {
        dialog->deleteLater();
        return std::nullopt;
    }
    return std::make_tuple(
            QDir::cleanPath(dialog->path->text().toLower()),
            dialog->useArchiveVPK && dialog->useArchiveVPK->checkState() == Qt::Checked,
            dialog->preloadBytes ? dialog->preloadBytes->value() : 0
    );
}
