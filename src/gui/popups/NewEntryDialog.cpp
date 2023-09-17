#include "NewEntryDialog.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSettings>
#include <QSpinBox>

#include "../Options.h"

NewEntryDialog::NewEntryDialog(QWidget* parent, const QString& prefilledPath)
        : QDialog(parent) {
    QSettings options;
    const bool advancedMode = options.value(OPT_ADV_MODE).toBool();

    this->setModal(true);
    this->setWindowTitle(advancedMode ? tr("Advanced New File Options") : tr("New File Options"));

    auto* layout = new QFormLayout(this);

    auto* pathLineEditLabel = new QLabel(tr("The path of the file in the VPK:\n(e.g. \"materials/cable.vmt\")"), this);
    this->path = new QLineEdit(this);
    this->path->setText(prefilledPath);
    layout->addRow(pathLineEditLabel, this->path);

    if (advancedMode) {
        auto* useArchiveVPKLabel = new QLabel(tr("Save this entry to a new numbered\narchive instead of the dir VPK:"), this);
        this->useArchiveVPK = new QCheckBox(this);
        layout->addRow(useArchiveVPKLabel, this->useArchiveVPK);

        auto* preloadBytesLabel = new QLabel(tr("Set the bytes of the file to preload:\n(From 0 to 1023 bytes)"), this);
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

std::optional<std::tuple<QString, bool, int>> NewEntryDialog::getNewEntryOptions(QWidget* parent, const QString& prefilledPath) {
    auto* dialog = new NewEntryDialog(parent, prefilledPath);
    int ret = dialog->exec();
    if (ret != QDialog::Accepted) {
        dialog->deleteLater();
        return std::nullopt;
    }
    return std::make_tuple(
            dialog->path->text(),
            dialog->useArchiveVPK && dialog->useArchiveVPK->checkState() == Qt::Checked,
            dialog->preloadBytes ? dialog->preloadBytes->value() : 0
    );
}
