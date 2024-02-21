#include "VTFPreview.h"

#include <utility>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QSlider>
#include <QSpinBox>
#include <QWheelEvent>

using namespace VTFLib;

namespace {

QString vtfFormatToString(VTFImageFormat format) {
	static const QMap<VTFImageFormat, QString> formatToString = {
			{ IMAGE_FORMAT_RGBA8888, "RGBA8888" },
			{ IMAGE_FORMAT_ABGR8888, "ABGR8888" },
			{ IMAGE_FORMAT_RGB888, "RGB888" },
			{ IMAGE_FORMAT_BGR888, "BGR888" },
			{ IMAGE_FORMAT_RGB565, "RGB565" },
			{ IMAGE_FORMAT_I8, "I8" },
			{ IMAGE_FORMAT_IA88, "IA88" },
			{ IMAGE_FORMAT_P8, "P8" },
			{ IMAGE_FORMAT_A8, "A8" },
			{ IMAGE_FORMAT_RGB888_BLUESCREEN, "RGB888_BLUESCREEN" },
			{ IMAGE_FORMAT_BGR888_BLUESCREEN, "BGR888_BLUESCREEN" },
			{ IMAGE_FORMAT_ARGB8888, "ARGB8888" },
			{ IMAGE_FORMAT_BGRA8888, "BGRA8888" },
			{ IMAGE_FORMAT_DXT1, "DXT1" },
			{ IMAGE_FORMAT_DXT3, "DXT3" },
			{ IMAGE_FORMAT_DXT5, "DXT5" },
			{ IMAGE_FORMAT_BGRX8888, "BGRX8888" },
			{ IMAGE_FORMAT_BGR565, "BGR565" },
			{ IMAGE_FORMAT_BGRX5551, "BGRX5551" },
			{ IMAGE_FORMAT_BGRA4444, "BGRA4444" },
			{ IMAGE_FORMAT_DXT1_ONEBITALPHA, "DXT1_ONEBITALPHA" },
			{ IMAGE_FORMAT_BGRA5551, "BGRA5551" },
			{ IMAGE_FORMAT_UV88, "UV88" },
			{ IMAGE_FORMAT_UVWQ8888, "UVWQ8888" },
			{ IMAGE_FORMAT_RGBA16161616F, "RGBA16161616F" },
			{ IMAGE_FORMAT_RGBA16161616, "RGBA16161616" },
			{ IMAGE_FORMAT_UVLX8888, "UVLX8888" },
			{ IMAGE_FORMAT_R32F, "R32F" },
			{ IMAGE_FORMAT_RGB323232F, "RGB323232F" },
			{ IMAGE_FORMAT_RGBA32323232F, "RGBA32323232F" },

			{ IMAGE_FORMAT_NV_NULL, "NV_NULL" },

			{ IMAGE_FORMAT_ATI2N, "ATI2N" },
			{ IMAGE_FORMAT_ATI1N, "ATI1N" },

			{ IMAGE_FORMAT_BC7, "BC7" },
	};
	if (!formatToString.contains(format)) {
		return QObject::tr("Unknown");
	}
	return formatToString[format];
}

} // namespace

VTFWidget::VTFWidget(QWidget* parent)
        : QWidget(parent)
        , currentFace(1)
        , currentFrame(1)
        , currentMip(0)
        , alphaEnabled(false)
        , tileEnabled(false)
        , zoom(1.f) {}

void VTFWidget::setData(const std::vector<std::byte>& data) {
    this->vtf = std::make_unique<VTFLib::CVTFFile>();
    this->vtf->Load(data.data(), static_cast<vlUInt>(data.size()));
    this->decodeImage(1, 1, 0, this->alphaEnabled);
    this->zoom = 1.f;
}

void VTFWidget::setFrame(int frame) {
    this->decodeImage(this->currentFace, frame, this->currentMip, this->alphaEnabled);
}

void VTFWidget::setFace(int face) {
    this->decodeImage(face, this->currentFrame, this->currentMip, this->alphaEnabled);
}

void VTFWidget::setMip(int mip) {
    this->decodeImage(this->currentFace, this->currentFrame, mip, this->alphaEnabled);
}

void VTFWidget::setAlphaEnabled(bool alpha) {
    this->decodeImage(this->currentFace, this->currentFrame, this->currentMip, alpha);
}

void VTFWidget::setTileEnabled(bool tile) {
    this->tileEnabled = tile;
}

void VTFWidget::setZoom(int zoom_) {
    this->zoom = static_cast<float>(zoom_) / 100.f;
}

int VTFWidget::getMaxFrame() const {
    return static_cast<int>(this->vtf->GetFrameCount());
}

int VTFWidget::getMaxFace() const {
    return static_cast<int>(this->vtf->GetFaceCount());
}

int VTFWidget::getMaxMip() const {
    return static_cast<int>(this->vtf->GetMipmapCount());
}

bool VTFWidget::hasAlpha() const {
    return CVTFFile::GetImageFormatInfo(this->vtf->GetFormat()).uiAlphaBitsPerPixel > 0;
}

bool VTFWidget::getAlphaEnabled() const {
    return this->alphaEnabled;
}

bool VTFWidget::getTileEnabled() const {
    return this->tileEnabled;
}

float VTFWidget::getZoom() const {
    return this->zoom;
}

QString VTFWidget::getVersion() const {
	return QString::number(this->vtf->GetMajorVersion()) + "." + QString::number(this->vtf->GetMinorVersion());
}

QString VTFWidget::getFormat() const {
	return ::vtfFormatToString(this->vtf->GetFormat());
}

int VTFWidget::getAuxCompression() const {
	if (this->vtf->GetMajorVersion() < 7 || this->vtf->GetMinorVersion() < 6) {
		return -1;
	}
	return this->vtf->GetAuxCompressionLevel();
}

// Taken directly from vtex2, thanks!
void VTFWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);

    if (!this->vtf) {
        return;
    }

    // Compute draw size for this mip, frame, etc
    vlUInt imageWidth, imageHeight, imageDepth;
    CVTFFile::ComputeMipmapDimensions(
            this->vtf->GetWidth(), this->vtf->GetHeight(), this->vtf->GetDepth(),
            this->currentMip, imageWidth, imageHeight, imageDepth);

    float realZoom = powf(2, static_cast<float>(this->currentMip)) * this->zoom;

    int zoomedXPos = (this->width() - static_cast<int>(static_cast<float>(imageWidth) * realZoom)) / 2;
    int zoomedYPos = (this->height() - static_cast<int>(static_cast<float>(imageHeight) * realZoom)) / 2;
    int zoomedWidth = static_cast<int>(static_cast<float>(this->image.width()) * realZoom);
    int zoomedHeight = static_cast<int>(static_cast<float>(this->image.height()) * realZoom);

    QRect sourceRect(0, 0, this->image.width(), this->image.height());

    if (!this->tileEnabled) {
        painter.drawImage(QRect(zoomedXPos, zoomedYPos, zoomedWidth, zoomedHeight), this->image, sourceRect);
        return;
    }
    for (int i = -zoomedWidth; i <= zoomedWidth; i += zoomedWidth) {
        for (int j = -zoomedHeight; j <= zoomedHeight; j += zoomedHeight) {
            painter.drawImage(QRect(zoomedXPos + i, zoomedYPos + j, zoomedWidth, zoomedHeight), this->image, sourceRect);
        }
    }
}

void VTFWidget::decodeImage(int face, int frame, int mip, bool alpha) {
	auto result = VTFDecoder::decodeImage(*this->vtf, face, frame, mip, alpha);
	if (!result) {
		this->image = QImage();
		return;
	}
	this->vtfData = std::move(result.value());
    this->image = QImage(reinterpret_cast<uchar*>(this->vtfData.data.get()), static_cast<int>(this->vtfData.width), static_cast<int>(this->vtfData.height), this->vtfData.format);
    this->currentFace = face;
    this->currentFrame = frame;
    this->currentMip = mip;
    this->alphaEnabled = alpha;
}

VTFPreview::VTFPreview(QWidget* parent)
        : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);

    this->vtf = new VTFWidget(this);
    layout->addWidget(this->vtf);

    auto* controls = new QWidget(this);
	controls->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
    layout->addWidget(controls);

    auto* controlsLayout = new QVBoxLayout(controls);

    auto* frameSpinParent = new QWidget(controls);
    auto* frameSpinLayout = new QHBoxLayout(frameSpinParent);
    auto* frameSpinLabel = new QLabel(tr("Frame"), frameSpinParent);
    frameSpinLayout->addWidget(frameSpinLabel);
    this->frameSpin = new QSpinBox(frameSpinParent);
    this->frameSpin->setMinimum(1);
    QObject::connect(this->frameSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [&] {
        this->vtf->setFrame(this->frameSpin->value());
        this->vtf->repaint();
    });
    frameSpinLayout->addWidget(this->frameSpin);
    controlsLayout->addWidget(frameSpinParent);

    auto* faceSpinParent = new QWidget(controls);
    auto* faceSpinLayout = new QHBoxLayout(faceSpinParent);
    auto* faceSpinLabel = new QLabel(tr("Face"), faceSpinParent);
    faceSpinLayout->addWidget(faceSpinLabel);
    this->faceSpin = new QSpinBox(controls);
    this->faceSpin->setMinimum(1);
    QObject::connect(this->frameSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [&] {
        this->vtf->setFace(this->faceSpin->value());
        this->vtf->repaint();
    });
    faceSpinLayout->addWidget(this->faceSpin);
    controlsLayout->addWidget(faceSpinParent);

    auto* mipSpinParent = new QWidget(controls);
    auto* mipSpinLayout = new QHBoxLayout(mipSpinParent);
    auto* mipSpinLabel = new QLabel(tr("Mip"), mipSpinParent);
    mipSpinLayout->addWidget(mipSpinLabel);
    this->mipSpin = new QSpinBox(controls);
    this->mipSpin->setMinimum(0);
    QObject::connect(this->mipSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [&] {
        this->vtf->setMip(this->mipSpin->value());
        this->vtf->repaint();
    });
    mipSpinLayout->addWidget(this->mipSpin);
    controlsLayout->addWidget(mipSpinParent);

    auto* alphaCheckBoxParent = new QWidget(controls);
    auto* alphaCheckBoxLayout = new QHBoxLayout(alphaCheckBoxParent);
    auto* alphaCheckBoxLabel = new QLabel(tr("Alpha"), alphaCheckBoxParent);
    alphaCheckBoxLayout->addWidget(alphaCheckBoxLabel);
    this->alphaCheckBox = new QCheckBox(controls);
    QObject::connect(this->alphaCheckBox, &QCheckBox::stateChanged, this, [&] {
        this->vtf->setAlphaEnabled(this->alphaCheckBox->isChecked());
        this->vtf->repaint();
    });
    alphaCheckBoxLayout->addWidget(this->alphaCheckBox, 0, Qt::AlignHCenter);
    controlsLayout->addWidget(alphaCheckBoxParent);

    auto* tileCheckBoxParent = new QWidget(controls);
    auto* tileCheckBoxLayout = new QHBoxLayout(tileCheckBoxParent);
    auto* tileCheckBoxLabel = new QLabel(tr("Tile"), tileCheckBoxParent);
    tileCheckBoxLayout->addWidget(tileCheckBoxLabel);
    this->tileCheckBox = new QCheckBox(controls);
    QObject::connect(this->tileCheckBox, &QCheckBox::stateChanged, this, [&] {
        this->vtf->setTileEnabled(this->tileCheckBox->isChecked());
        this->vtf->repaint();
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
        this->vtf->setZoom(this->zoomSlider->value());
        this->vtf->repaint();
    });
    zoomSliderLayout->addWidget(this->zoomSlider, 0, Qt::AlignHCenter);
    controlsLayout->addWidget(zoomSliderParent);

    this->versionLabel = new QLabel(controls);
	this->versionLabel->setAlignment(Qt::AlignRight);
    controlsLayout->addWidget(this->versionLabel);

    this->imageFormatLabel = new QLabel(controls);
	this->imageFormatLabel->setAlignment(Qt::AlignRight);
    controlsLayout->addWidget(this->imageFormatLabel);

    this->compressionLevelLabel = new QLabel(controls);
	this->compressionLevelLabel->setAlignment(Qt::AlignRight);
    controlsLayout->addWidget(this->compressionLevelLabel);
}

void VTFPreview::setData(const std::vector<std::byte>& data) const {
    this->vtf->setData(data);

    this->frameSpin->setMaximum(this->vtf->getMaxFrame());
    this->frameSpin->setValue(1);
    this->frameSpin->setDisabled(this->vtf->getMaxFrame() == 1);

    this->faceSpin->setMaximum(this->vtf->getMaxFace());
    this->faceSpin->setValue(1);
    this->faceSpin->setDisabled(this->vtf->getMaxFace() == 1);

    this->mipSpin->setMaximum(this->vtf->getMaxMip() - 1);
    this->mipSpin->setValue(0);
    this->mipSpin->setDisabled(this->vtf->getMaxMip() == 1);

    this->alphaCheckBox->setChecked(false);
    this->alphaCheckBox->setDisabled(!this->vtf->hasAlpha());

    this->tileCheckBox->setChecked(false);

    this->zoomSlider->setValue(100);

    this->versionLabel->setText(tr("Version: %1").arg(this->vtf->getVersion()));

    this->imageFormatLabel->setText(tr("Format: %1").arg(this->vtf->getFormat()));

    this->compressionLevelLabel->setText(tr("Compression: %1").arg(this->vtf->getAuxCompression()));
	this->compressionLevelLabel->setVisible(this->vtf->getAuxCompression() >= 0);
}

void VTFPreview::wheelEvent(QWheelEvent* event) {
    if (QPoint numDegrees = event->angleDelta() / 8; !numDegrees.isNull()) {
        this->zoomSlider->setValue(this->zoomSlider->value() + numDegrees.y());
    }
    event->accept();
}
