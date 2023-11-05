#include "ImagePreview.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QSlider>
#include <QWheelEvent>

Image::Image(QWidget* parent)
        : QWidget(parent)
        , zoom(1.f) {}

void Image::setImage(const std::vector<std::byte>& data) {
    this->image.loadFromData(data);
    this->zoom = 1.f;
}

void Image::setZoom(int zoom_) {
    this->zoom = static_cast<float>(zoom_) / 100.f;
}

void Image::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);

    int imageWidth = this->image.width(), imageHeight = this->image.height();

    QRect target = QRect((this->width() - (int) ((float) imageWidth * zoom)) / 2,
                         (this->height() - (int) ((float) imageHeight * zoom)) / 2,
                         (int) ((float) this->image.width() * zoom),
                         (int) ((float) this->image.height() * zoom));
    painter.drawImage(target, this->image, QRect(0, 0, this->image.width(), this->image.height()));
}

ImagePreview::ImagePreview(QWidget* parent)
        : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);

    this->image = new Image(this);
    layout->addWidget(this->image);

    auto* controls = new QWidget(this);
    controls->setFixedWidth(120);
    layout->addWidget(controls);

    auto* controlsLayout = new QVBoxLayout(controls);

    this->zoomSlider = new QSlider(controls);
    this->zoomSlider->setMinimum(20);
    this->zoomSlider->setMaximum(800);
    this->zoomSlider->setValue(100);
    QObject::connect(this->zoomSlider, &QSlider::valueChanged, [&] {
        this->image->setZoom(this->zoomSlider->value());
        this->image->repaint();
    });
    controlsLayout->addWidget(this->zoomSlider, 0, Qt::AlignHCenter);
}

void ImagePreview::setImage(const std::vector<std::byte>& data) {
    this->image->setImage(data);
    this->zoomSlider->setValue(100);
}

void ImagePreview::wheelEvent(QWheelEvent* event) {
    if (QPoint numDegrees = event->angleDelta() / 8; !numDegrees.isNull()) {
        this->zoomSlider->setValue(this->zoomSlider->value() + numDegrees.y());
    }
    event->accept();
}
