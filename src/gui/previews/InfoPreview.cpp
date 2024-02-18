#include "InfoPreview.h"

#include <QLabel>
#include <QVBoxLayout>

InfoPreview::InfoPreview(const QPixmap& icon, const QString& text, QWidget* parent)
		: QWidget(parent) {
	this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	auto* layout = new QVBoxLayout(this);

	this->image = new QLabel(this);
	this->image->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio));
	layout->addWidget(this->image, 0, Qt::AlignHCenter | Qt::AlignBottom);

	this->error = new QLabel(this);
	this->error->setText(text);
	this->error->setAlignment(Qt::AlignHCenter);
	layout->addWidget(this->error, 0, Qt::AlignHCenter | Qt::AlignTop);
}

InfoPreview::InfoPreview(QWidget *parent)
		: InfoPreview({":/error.png"}, "") {}

void InfoPreview::setData(const QPixmap& icon, const QString& text) {
	this->image->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio));
	this->error->setText(text);
}
