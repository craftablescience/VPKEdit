#include "NewEntryDialog.h"

#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>

#include "../Options.h"

NewEntryDialog::NewEntryDialog(bool isDir, const QString& prefilledPath, QWidget* parent)
        : QDialog(parent) {
    const bool advancedMode = Options::get<bool>(OPT_ADV_MODE);

    this->setModal(true);
    this->setWindowTitle(isDir
            ? (advancedMode ? tr("Advanced New Folder Options") : tr("New Folder Options"))
            : (advancedMode ? tr("Advanced New File Options") : tr("New File Options")));

    auto* layout = new QFormLayout(this);

    auto* pathLineEditLabel = new QLabel(
            isDir ? tr("The path of the folder in the VPK:\n(e.g. \"materials/dev\")") : tr("The path of the file in the VPK:\n(e.g. \"materials/cable.vmt\")"),
            this);
    this->path = new QLineEdit(this);
    this->path->setText(prefilledPath);
    layout->addRow(pathLineEditLabel, this->path);

    if (advancedMode) {
        auto* useArchiveVPKLabel = new QLabel(
                isDir ? tr("Save this file to a new numbered archive\ninstead of the directory VPK:") : tr("Save each file to a new numbered archive\ninstead of the directory VPK:"),
                this);
        this->useArchiveVPK = new QCheckBox(this);
        layout->addRow(useArchiveVPKLabel, this->useArchiveVPK);

        auto* preloadBytesLabel = new QLabel(
                isDir ? tr("Set the bytes of the file to preload:\n(From 0 to 1023 bytes)") : tr("Set the bytes of each file to preload:\n(From 0 to 1023 bytes)"),
                this);
        this->preloadBytes = new QSpinBox(this);
        this->preloadBytes->setMinimum(0);
        this->preloadBytes->setMaximum(1023);
        this->preloadBytes->setValue(0);
        layout->addRow(preloadBytesLabel, this->preloadBytes);
    } else {
        this->useArchiveVPK = nullptr;
        this->preloadBytes = nullptr;
    }

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &NewEntryDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &NewEntryDialog::reject);
}

std::optional<std::tuple<QString, bool, int>> NewEntryDialog::getNewEntryOptions(bool isDir, const QString& prefilledPath, QWidget* parent) {
    auto* dialog = new NewEntryDialog(isDir, prefilledPath, parent);
    int ret = dialog->exec();
    if (ret != QDialog::Accepted) {
        dialog->deleteLater();
        return std::nullopt;
    }
    return std::make_tuple(
            QDir::cleanPath(dialog->path->text()),
            dialog->useArchiveVPK && dialog->useArchiveVPK->checkState() == Qt::Checked,
            dialog->preloadBytes ? dialog->preloadBytes->value() : 0
    );
}
