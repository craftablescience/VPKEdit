#include "VTFPreview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>

using namespace VTFLib;

VTFImage::VTFImage(QWidget* parent)
        : QWidget(parent)
        , currentFace(1)
        , currentFrame(1)
        , currentMip(0)
        , currentAlphaEnabled(true)
        , zoom(1.f) {}

void VTFImage::setImage(const std::vector<std::byte>& data) {
    this->vtf = std::make_unique<VTFLib::CVTFFile>();
    this->vtf->Load(data.data(), data.size());
    this->decodeImage(1, 1, 0, this->currentAlphaEnabled);
    this->zoom = 1.f;
}

void VTFImage::setFrame(int frame) {
    this->decodeImage(this->currentFace, frame, this->currentMip, this->currentAlphaEnabled);
}

void VTFImage::setFace(int face) {
    this->decodeImage(face, this->currentFrame, this->currentMip, this->currentAlphaEnabled);
}

void VTFImage::setMip(int mip) {
    this->decodeImage(this->currentFace, this->currentFrame, mip, this->currentAlphaEnabled);
}

void VTFImage::setAlpha(bool alpha) {
    this->decodeImage(this->currentFace, this->currentFrame, this->currentMip, alpha);
}

void VTFImage::setZoom(int zoom_) {
    this->zoom = static_cast<float>(zoom_) / 100.f;
}

int VTFImage::getMaxFrame() {
    return (int) this->vtf->GetFrameCount();
}

int VTFImage::getMaxFace() {
    return (int) this->vtf->GetFaceCount();
}

int VTFImage::getMaxMip() {
    return (int) this->vtf->GetMipmapCount();
}

// Taken directly from vtex2, thanks!
void VTFImage::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    if (!this->vtf) {
        return;
    }

    // Compute draw size for this mip, frame, etc
    vlUInt imageWidth, imageHeight, imageDepth;
    CVTFFile::ComputeMipmapDimensions(
            this->vtf->GetWidth(), this->vtf->GetHeight(), this->vtf->GetDepth(),
            this->currentMip, imageWidth, imageHeight, imageDepth);

    auto realZoom = pow(2, this->currentMip) * this->zoom;

    QRect target = QRect((this->width() - (int) ((float) imageWidth * realZoom)) / 2,
                         (this->height() - (int) ((float) imageHeight * realZoom)) / 2,
                         (int) ((float) this->image.width() * realZoom),
                         (int) ((float) this->image.height() * realZoom));
    painter.drawImage(target, this->image, QRect(0, 0, this->image.width(), this->image.height()));
}

void VTFImage::decodeImage(int face, int frame, int mip, bool alphaEnabled) {
    // Compute draw size for this mip, frame, etc
    vlUInt imageWidth, imageHeight, imageDepth;
    CVTFFile::ComputeMipmapDimensions(
            this->vtf->GetWidth(), this->vtf->GetHeight(), this->vtf->GetDepth(),
            mip, imageWidth, imageHeight, imageDepth);

    const bool hasAlpha = CVTFFile::GetImageFormatInfo(this->vtf->GetFormat()).uiAlphaBitsPerPixel > 0;
    const VTFImageFormat format = (hasAlpha && alphaEnabled) ? IMAGE_FORMAT_RGBA8888 : IMAGE_FORMAT_RGB888;
    auto size = CVTFFile::ComputeMipmapSize(this->vtf->GetWidth(), this->vtf->GetHeight(), 1, mip, format);

    // This buffer needs to persist- QImage does not own the mem you give it
    this->imageData.reset(new std::byte[size]);

    bool ok = CVTFFile::Convert(
            this->vtf->GetData(frame, face, 0, mip), (vlByte*) this->imageData.get(), imageWidth, imageHeight, this->vtf->GetFormat(), format);
    if (!ok) {
        return;
    }

    this->image = QImage(
            (uchar*) this->imageData.get(), (int) imageWidth, (int) imageHeight, (hasAlpha && alphaEnabled) ? QImage::Format_RGBA8888 : QImage::Format_RGB888);
    this->currentFace = face;
    this->currentFrame = frame;
    this->currentMip = mip;
    this->currentAlphaEnabled = alphaEnabled;
}

VTFPreview::VTFPreview(QWidget* parent)
        : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);

    this->image = new VTFImage(this);
    layout->addWidget(this->image);

    auto* controls = new QWidget(this);
    controls->setFixedWidth(120);
    layout->addWidget(controls);

    auto* controlsLayout = new QVBoxLayout(controls);

    auto* frameSpinParent = new QWidget(controls);
    auto* frameSpinLayout = new QHBoxLayout(frameSpinParent);
    auto* frameSpinLabel = new QLabel(tr("Frame"), frameSpinParent);
    frameSpinLayout->addWidget(frameSpinLabel);
    this->frameSpin = new QSpinBox(frameSpinParent);
    this->frameSpin->setMinimum(1);
    connect(this->frameSpin, QOverload<int>::of(&QSpinBox::valueChanged), [&] {
        this->image->setFrame(this->frameSpin->value());
        this->image->repaint();
    });
    frameSpinLayout->addWidget(this->frameSpin);
    controlsLayout->addWidget(frameSpinParent);

    auto* faceSpinParent = new QWidget(controls);
    auto* faceSpinLayout = new QHBoxLayout(faceSpinParent);
    auto* faceSpinLabel = new QLabel(tr("Face"), faceSpinParent);
    faceSpinLayout->addWidget(faceSpinLabel);
    this->faceSpin = new QSpinBox(controls);
    this->faceSpin->setMinimum(1);
    connect(this->frameSpin, QOverload<int>::of(&QSpinBox::valueChanged), [&] {
        this->image->setFace(this->faceSpin->value());
        this->image->repaint();
    });
    faceSpinLayout->addWidget(this->faceSpin);
    controlsLayout->addWidget(faceSpinParent);

    auto* mipSpinParent = new QWidget(controls);
    auto* mipSpinLayout = new QHBoxLayout(mipSpinParent);
    auto* mipSpinLabel = new QLabel(tr("Mip"), mipSpinParent);
    mipSpinLayout->addWidget(mipSpinLabel);
    this->mipSpin = new QSpinBox(controls);
    this->mipSpin->setMinimum(0);
    connect(this->mipSpin, QOverload<int>::of(&QSpinBox::valueChanged), [&] {
        this->image->setMip(this->mipSpin->value());
        this->image->repaint();
    });
    mipSpinLayout->addWidget(this->mipSpin);
    controlsLayout->addWidget(mipSpinParent);

    auto* alphaCheckBoxParent = new QWidget(controls);
    auto* alphaCheckBoxayout = new QHBoxLayout(alphaCheckBoxParent);
    auto* alphaCheckBoxLabel = new QLabel(tr("Show Alpha"), alphaCheckBoxParent);
    alphaCheckBoxayout->addWidget(alphaCheckBoxLabel);
    this->alphaCheckBox = new QCheckBox(controls);
    this->alphaCheckBox->setChecked(true);
    connect(this->alphaCheckBox, QOverload<int>::of(&QCheckBox::stateChanged), [&] {
        this->image->setAlpha(this->alphaCheckBox->isChecked());
        this->image->repaint();
    });
    alphaCheckBoxayout->addWidget(this->alphaCheckBox);
    controlsLayout->addWidget(alphaCheckBoxParent);

    this->zoomSlider = new QSlider(controls);
    this->zoomSlider->setMinimum(20);
    this->zoomSlider->setMaximum(800);
    this->zoomSlider->setValue(100);
    connect(this->zoomSlider, &QSlider::valueChanged, [&] {
        this->image->setZoom(this->zoomSlider->value());
        this->image->repaint();
    });
    controlsLayout->addWidget(this->zoomSlider, 0, Qt::AlignHCenter);
}

void VTFPreview::setImage(const std::vector<std::byte>& data) {
    this->image->setImage(data);

    this->frameSpin->setMaximum(this->image->getMaxFrame());
    this->frameSpin->setValue(1);
    this->frameSpin->setDisabled(this->image->getMaxFrame() == 1);

    this->faceSpin->setMaximum(this->image->getMaxFace());
    this->faceSpin->setValue(1);
    this->faceSpin->setDisabled(this->image->getMaxFace() == 1);

    this->mipSpin->setMaximum(this->image->getMaxMip() - 1);
    this->mipSpin->setValue(0);
    this->mipSpin->setDisabled(this->image->getMaxMip() == 1);

    this->zoomSlider->setValue(100);
}
