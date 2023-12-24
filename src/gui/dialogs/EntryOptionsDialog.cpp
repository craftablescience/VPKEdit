#include "EntryOptionsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

#include "../config/Options.h"

EntryOptionsDialog::EntryOptionsDialog(bool edit, bool isDir, const QString& prefilledPath, bool prefilledUseDirVPK, int prefilledPreloadBytes, QWidget* parent)
        : QDialog(parent) {
    const bool advancedFileProps = Options::get<bool>(OPT_ADVANCED_FILE_PROPS);

    this->setModal(true);
    this->setWindowTitle(tr("%1%2 %3").arg(advancedFileProps ? "(Advanced) " : "", edit ? "Edit" : "New", isDir ? "Folder" : "File"));
	// This works well enough without messing around with QFontMetrics
	this->setMinimumWidth(static_cast<int>(130 + (prefilledPath.length() * 8)));

    auto* layout = new QFormLayout(this);

    auto* pathLineEditLabel = new QLabel(
            isDir ? tr("The path of the folder in the VPK:\n(e.g. \"materials/dev\")") : tr("The path of the file in the VPK:\n(e.g. \"materials/cable.vmt\")"),
            this);
    this->path = new QLineEdit(this);
    this->path->setText(prefilledPath);
    layout->addRow(pathLineEditLabel, this->path);

    if (advancedFileProps) {
        auto* useDirVPKLabel = new QLabel(
                isDir ? tr("Save this file to a new numbered archive\ninstead of the directory VPK:") : tr("Save each file to a new numbered archive\ninstead of the directory VPK:"),
                this);
        this->useDirVPK = new QCheckBox(this);
        this->useDirVPK->setCheckState(prefilledUseDirVPK ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
        layout->addRow(useDirVPKLabel, this->useDirVPK);

        auto* preloadBytesLabel = new QLabel(
                isDir ? tr("Set the bytes of the file to preload:\n(From 0 to 1023 bytes)") : tr("Set the bytes of each file to preload:\n(From 0 to 1023 bytes)"),
                this);
        this->preloadBytes = new QSpinBox(this);
        this->preloadBytes->setMinimum(0);
        this->preloadBytes->setMaximum(1023);
        this->preloadBytes->setValue(prefilledPreloadBytes);
        layout->addRow(preloadBytesLabel, this->preloadBytes);
    } else {
        this->useDirVPK = nullptr;
        this->preloadBytes = nullptr;
    }

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &EntryOptionsDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &EntryOptionsDialog::reject);
}

std::optional<std::tuple<QString, bool, int>> EntryOptionsDialog::getEntryOptions(bool edit, bool isDir, const QString& prefilledPath, bool prefilledUseDirVPK, int prefilledPreloadBytes, QWidget* parent) {
    auto* dialog = new EntryOptionsDialog(edit, isDir, prefilledPath, prefilledUseDirVPK, prefilledPreloadBytes, parent);
    int ret = dialog->exec();
    if (ret != QDialog::Accepted) {
        dialog->deleteLater();
        return std::nullopt;
    }
    return std::make_tuple(
            QDir::cleanPath(dialog->path->text()),
            !dialog->useDirVPK || dialog->useDirVPK->checkState() == Qt::Checked,
            dialog->preloadBytes ? dialog->preloadBytes->value() : 0
    );
}
