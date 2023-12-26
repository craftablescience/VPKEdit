#include "AbstractInfoPreview.h"

#include <QLabel>
#include <QVBoxLayout>

AbstractInfoPreview::AbstractInfoPreview(const QPixmap& icon, const QString& text, QWidget* parent)
        : QWidget(parent) {
    this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    auto* layout = new QVBoxLayout(this);

    this->image = new QLabel(this);
    this->image->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio));
    layout->addWidget(this->image, 0, Qt::AlignHCenter | Qt::AlignBottom);

    this->error = new QLabel(this);
    this->error->setText(text);
    layout->addWidget(this->error, 0, Qt::AlignHCenter | Qt::AlignTop);
}
