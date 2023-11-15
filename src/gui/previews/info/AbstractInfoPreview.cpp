#include "AbstractInfoPreview.h"

#include <QHBoxLayout>
#include <QLabel>

AbstractInfoPreview::AbstractInfoPreview(const QPixmap& icon, const QString& text, QWidget* parent)
        : QWidget(parent) {
    this->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Minimum);

    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(16);

    this->image = new QLabel(this);
    this->image->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio));
    layout->addWidget(this->image, Qt::AlignCenter);

    this->error = new QLabel(this);
    this->error->setText(text);
    layout->addWidget(this->error, Qt::AlignLeft);
}
