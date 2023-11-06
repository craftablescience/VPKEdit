#include "AbstractInfoPreview.h"

#include <QHBoxLayout>
#include <QLabel>

AbstractInfoPreview::AbstractInfoPreview(const QPixmap& icon, const QString& text, QWidget* parent)
        : QWidget(parent) {
    this->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Minimum);

    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(16);

    auto* image = new QLabel(this);
    image->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio));
    layout->addWidget(image, Qt::AlignCenter);

    auto* label = new QLabel(this);
    label->setText(text);
    layout->addWidget(label, Qt::AlignLeft);
}
