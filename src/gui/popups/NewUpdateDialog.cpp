#include "NewUpdateDialog.h"

#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "../Config.h"

NewUpdateDialog::NewUpdateDialog(const QString& releaseLink, const QString& version, QWidget* parent)
        : QDialog(parent) {
    this->setModal(true);
    this->setWindowTitle(tr("New Update Found"));

    auto* layout = new QVBoxLayout(this);

    auto* label1 = new QLabel(tr("### There is a new update available!"), this);
    label1->setTextFormat(Qt::MarkdownText);
    layout->addWidget(label1);

    auto* label2 = new QLabel(tr("Currently running: v" VPKEDIT_PROJECT_VERSION "\nLatest version: %1").arg(version), this);
    layout->addWidget(label2);

    auto* label3 = new QLabel(tr("[Click this link to download the new version!](%1)").arg(releaseLink), this);
    label3->setTextFormat(Qt::MarkdownText);
    label3->setOpenExternalLinks(true);
    layout->addWidget(label3);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &NewUpdateDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &NewUpdateDialog::reject);
}

void NewUpdateDialog::getNewUpdatePrompt(const QString& releaseLink, const QString& version, QWidget* parent) {
    auto* dialog = new NewUpdateDialog(releaseLink, version, parent);
    dialog->exec();
    dialog->deleteLater();
}
