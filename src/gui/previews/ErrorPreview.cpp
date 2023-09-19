#include "ErrorPreview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>

ErrorPreview::ErrorPreview(QWidget* parent)
        : QWidget(parent) {
    this->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Minimum);

    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(16);

    auto* warningImage = new QLabel(this);
    warningImage->setPixmap(QPixmap(":/error.png"));
    layout->addWidget(warningImage, Qt::AlignCenter);

    auto* warningLabel = new QLabel(this);
    warningLabel->setText(tr("Failed to read file contents!\nPlease ensure that a game or another application is not using the VPK."));
    layout->addWidget(warningLabel, Qt::AlignLeft);
}
