#include "VPKVersionDialog.h"

#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFormLayout>

#include <vpkedit/VPK.h>

using namespace vpkedit;

VPKVersionDialog::VPKVersionDialog(bool exists, std::uint32_t startVersion, QWidget* parent)
        : QDialog(parent) {
    this->setModal(true);
    this->setWindowTitle(exists ? tr("Set VPK Version") : tr("New VPK Options"));

    auto* layout = new QFormLayout(this);

    auto* versionLabel = new QLabel(tr("Version:"), this);
    this->version = new QComboBox(this);
    this->version->addItem(tr("v1"));
    this->version->addItem(tr("v2"));
    this->version->setCurrentIndex(static_cast<int>(startVersion) - 1);
    layout->addRow(versionLabel, this->version);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &VPKVersionDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &VPKVersionDialog::reject);
}

std::optional<std::tuple<std::uint32_t>> VPKVersionDialog::getVPKVersionOptions(bool exists, std::uint32_t startVersion, QWidget* parent) {
    auto* dialog = new VPKVersionDialog(exists, startVersion, parent);
    int ret = dialog->exec();
    if (ret != QDialog::Accepted) {
        dialog->deleteLater();
        return std::nullopt;
    }
    // v1 - 1, v2 - 2
    return std::make_tuple(dialog->version->currentIndex() + 1);
}
