#include "NewVPKDialog.h"

#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFormLayout>

#include <vpktool/VPK.h>

using namespace vpktool;

NewVPKDialog::NewVPKDialog(QWidget* parent)
        : QDialog(parent) {
    this->setModal(true);
    this->setWindowTitle(tr("New VPK Options"));

    auto* layout = new QFormLayout(this);

    auto* versionLabel = new QLabel(tr("Version:"), this);
    this->version = new QComboBox(this);
    this->version->addItem(tr("v1"));
    this->version->addItem(tr("v2"));
    this->version->addItem(tr("v2 (Counter-Strike 2)"));
    this->version->setCurrentIndex(1); // v2
    layout->addRow(versionLabel, this->version);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &NewVPKDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &NewVPKDialog::reject);
}

std::optional<std::tuple<int>> NewVPKDialog::getNewVPKOptions(QWidget* parent) {
    auto* dialog = new NewVPKDialog(parent);
    int ret = dialog->exec();
    if (ret != QDialog::Accepted) {
        dialog->deleteLater();
        return std::nullopt;
    }
    // v1 - 1, v2 - 2, CS2 - VPK_ID
    return std::make_tuple(dialog->version->currentIndex() == 2 ? VPK_ID : dialog->version->currentIndex() + 1);
}
