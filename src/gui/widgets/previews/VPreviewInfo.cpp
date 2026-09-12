#include "VPreviewInfo.hpp"

#include <QLabel>
#include <QVBoxLayout>

VPreviewInfo::VPreviewInfo(const QPixmap& icon, const QString& text, QWidget* parent) : QWidget{parent} {
	this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	auto* layout = new QVBoxLayout{this};
	layout->setContentsMargins(0,0,0,0);

	this->icon = new QLabel{this};
	this->icon->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	layout->addWidget(this->icon, 0, Qt::AlignHCenter | Qt::AlignBottom);

	this->text = new QLabel(this);
	this->text->setText(text);
	this->text->setTextFormat(Qt::MarkdownText);
	layout->addWidget(this->text, 0, Qt::AlignHCenter | Qt::AlignTop);
}

VPreviewInfo::VPreviewInfo(QWidget* parent) : VPreviewInfo({":/icons/error.png"}, "", parent) {}

void VPreviewInfo::setIcon(const QPixmap& icon_) const {
	this->icon->setPixmap(icon_.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void VPreviewInfo::setText(const QString& text_) const {
	this->text->setText(text_);
}
