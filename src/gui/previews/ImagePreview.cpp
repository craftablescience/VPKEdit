#include "ImagePreview.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QSlider>
#include <QWheelEvent>

ImageWidget::ImageWidget(QWidget* parent)
		: QWidget(parent)
		, alphaEnabled(false)
		, tileEnabled(false)
		, zoom(1.f) {}

void ImageWidget::setData(const std::vector<std::byte>& data) {
	this->image.loadFromData(data);
	this->zoom = 1.f;
}

void ImageWidget::setAlphaEnabled(bool alpha) {
	this->alphaEnabled = alpha;
}

void ImageWidget::setTileEnabled(bool tile) {
	this->tileEnabled = tile;
}

void ImageWidget::setZoom(int zoom_) {
	this->zoom = static_cast<float>(zoom_) / 100.f;
}

bool ImageWidget::hasAlpha() const {
	return this->image.hasAlphaChannel();
}

bool ImageWidget::getAlphaEnabled() const {
	return this->alphaEnabled;
}

bool ImageWidget::getTileEnabled() const {
	return this->tileEnabled;
}

float ImageWidget::getZoom() const {
	return this->zoom;
}

void ImageWidget::paintEvent(QPaintEvent* /*event*/) {
	QPainter painter(this);

	int imageWidth = this->image.width(), imageHeight = this->image.height();

	int zoomedXPos = (this->width() - static_cast<int>(static_cast<float>(imageWidth) * this->zoom)) / 2;
	int zoomedYPos = (this->height() - static_cast<int>(static_cast<float>(imageHeight) * this->zoom)) / 2;
	int zoomedWidth = static_cast<int>(static_cast<float>(this->image.width()) * this->zoom);
	int zoomedHeight = static_cast<int>(static_cast<float>(this->image.height()) * this->zoom);

	QRect sourceRect(0, 0, this->image.width(), this->image.height());

	const auto imageFlags = this->alphaEnabled
			? Qt::ImageConversionFlag::AutoColor | Qt::ImageConversionFlag::NoAlpha
			: Qt::ImageConversionFlag::AutoColor;

	if (!this->tileEnabled) {
		painter.drawImage(QRect(zoomedXPos, zoomedYPos, zoomedWidth, zoomedHeight), this->image, sourceRect, imageFlags);
		return;
	}
	for (int i = -zoomedWidth; i <= zoomedWidth; i += zoomedWidth) {
		for (int j = -zoomedHeight; j <= zoomedHeight; j += zoomedHeight) {
			painter.drawImage(QRect(zoomedXPos + i, zoomedYPos + j, zoomedWidth, zoomedHeight), this->image, sourceRect, imageFlags);
		}
	}
}

ImagePreview::ImagePreview(QWidget* parent)
		: QWidget(parent) {
	auto* layout = new QHBoxLayout(this);

	this->image = new ImageWidget(this);
	layout->addWidget(this->image);

	auto* controls = new QWidget(this);
	controls->setFixedWidth(115);
	layout->addWidget(controls);

	auto* controlsLayout = new QVBoxLayout(controls);

	auto* alphaCheckBoxParent = new QWidget(controls);
	auto* alphaCheckBoxLayout = new QHBoxLayout(alphaCheckBoxParent);
	auto* alphaCheckBoxLabel = new QLabel(tr("Alpha"), alphaCheckBoxParent);
	alphaCheckBoxLayout->addWidget(alphaCheckBoxLabel);
	this->alphaCheckBox = new QCheckBox(controls);
	QObject::connect(this->alphaCheckBox, &QCheckBox::stateChanged, this, [&] {
		this->image->setAlphaEnabled(this->alphaCheckBox->isChecked());
		this->image->repaint();
	});
	alphaCheckBoxLayout->addWidget(this->alphaCheckBox, 0, Qt::AlignHCenter);
	controlsLayout->addWidget(alphaCheckBoxParent);

	auto* tileCheckBoxParent = new QWidget(controls);
	auto* tileCheckBoxLayout = new QHBoxLayout(tileCheckBoxParent);
	auto* tileCheckBoxLabel = new QLabel(tr("Tile"), tileCheckBoxParent);
	tileCheckBoxLayout->addWidget(tileCheckBoxLabel);
	this->tileCheckBox = new QCheckBox(controls);
	QObject::connect(this->tileCheckBox, &QCheckBox::stateChanged, this, [&] {
		this->image->setTileEnabled(this->tileCheckBox->isChecked());
		this->image->repaint();
	});
	tileCheckBoxLayout->addWidget(this->tileCheckBox, 0, Qt::AlignHCenter);
	controlsLayout->addWidget(tileCheckBoxParent);

	auto* zoomSliderParent = new QWidget(controls);
	auto* zoomSliderLayout = new QHBoxLayout(zoomSliderParent);
	auto* zoomSliderLabel = new QLabel(tr("Zoom"), zoomSliderParent);
	zoomSliderLayout->addWidget(zoomSliderLabel);
	this->zoomSlider = new QSlider(controls);
	this->zoomSlider->setMinimum(20);
	this->zoomSlider->setMaximum(800);
	this->zoomSlider->setValue(100);
	QObject::connect(this->zoomSlider, &QSlider::valueChanged, this, [&] {
		this->image->setZoom(this->zoomSlider->value());
		this->image->repaint();
	});
	zoomSliderLayout->addWidget(this->zoomSlider, 0, Qt::AlignHCenter);
	controlsLayout->addWidget(zoomSliderParent);
}

void ImagePreview::setData(const std::vector<std::byte>& data) const {
	this->image->setData(data);

	this->alphaCheckBox->setChecked(false);
	this->alphaCheckBox->setDisabled(!this->image->hasAlpha());

	this->tileCheckBox->setChecked(false);

	this->zoomSlider->setValue(100);
}

void ImagePreview::wheelEvent(QWheelEvent* event) {
	if (QPoint numDegrees = event->angleDelta() / 8; !numDegrees.isNull()) {
		this->zoomSlider->setValue(this->zoomSlider->value() + numDegrees.y());
	}
	event->accept();
}
