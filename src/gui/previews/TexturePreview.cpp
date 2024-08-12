#include "TexturePreview.h"

#include <utility>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QSvgRenderer>
#include <QWheelEvent>

using namespace vtfpp;

namespace {

QString vtfFormatToString(ImageFormat format) {
	using enum ImageFormat;
	static const QMap<ImageFormat, QString> formatToString = {
		{ RGBA8888, "RGBA8888" },
		{ ABGR8888, "ABGR8888" },
		{ RGB888, "RGB888" },
		{ BGR888, "BGR888" },
		{ RGB565, "RGB565" },
		{ I8, "I8" },
		{ IA88, "IA88" },
		{ P8, "P8" },
		{ A8, "A8" },
		{ RGB888_BLUESCREEN, "RGB888_BLUESCREEN" },
		{ BGR888_BLUESCREEN, "BGR888_BLUESCREEN" },
		{ ARGB8888, "ARGB8888" },
		{ BGRA8888, "BGRA8888" },
		{ DXT1, "DXT1" },
		{ DXT3, "DXT3" },
		{ DXT5, "DXT5" },
		{ BGRX8888, "BGRX8888" },
		{ BGR565, "BGR565" },
		{ BGRX5551, "BGRX5551" },
		{ BGRA4444, "BGRA4444" },
		{ DXT1_ONE_BIT_ALPHA, "DXT1_ONEBITALPHA" },
		{ BGRA5551, "BGRA5551" },
		{ UV88, "UV88" },
		{ UVWQ8888, "UVWQ8888" },
		{ RGBA16161616F, "RGBA16161616F" },
		{ RGBA16161616, "RGBA16161616" },
		{ UVLX8888, "UVLX8888" },
		{ R32F, "R32F" },
		{ RGB323232F, "RGB323232F" },
		{ RGBA32323232F, "RGBA32323232F" },
		{ EMPTY, "NV_NULL" },
		{ ATI2N, "ATI2N" },
		{ ATI1N, "ATI1N" },
		{ BC7, "BC7" },
		{ BC6H, "BC6H" },
	};
	if (!formatToString.contains(format)) {
		return QObject::tr("Unknown");
	}
	return formatToString[format];
}

} // namespace

ITextureWidget::ITextureWidget(QWidget* parent)
		: QWidget(parent) {
	this->setContextMenuPolicy(Qt::CustomContextMenu);

	auto* contextMenu = new QMenu(this);
	auto* copyImageAction = contextMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("Copy Image"));

	QObject::connect(this, &VTFWidget::customContextMenuRequested, this, [this, contextMenu, copyImageAction](const QPoint& pos) {
		if (this->image.isNull()) {
			return;
		}
		auto* selectedAction = contextMenu->exec(this->mapToGlobal(pos));
		if (selectedAction == copyImageAction) {
			QApplication::clipboard()->setImage(this->image, QClipboard::Clipboard);
		}
	});
}

void ITextureWidget::setZoom(int zoom_) {
	this->zoom = static_cast<float>(zoom_) / 100.f;
}

void ImageWidget::setData(const std::vector<std::byte>& data) {
	this->image.loadFromData(data);
	this->zoom = 1.f;
}

QString ImageWidget::getFormat() const {
	// Add 7 for the length of "Format_"
	return QMetaEnum::fromType<QImage::Format>().valueToKey(this->image.format()) + 7;
}

void ImageWidget::paintEvent(QPaintEvent* /*event*/) {
	QPainter painter(this);

	float realZoom = static_cast<float>(1 << this->currentMip) * this->zoom;

	int zoomedXPos = (this->width() - static_cast<int>(static_cast<float>(this->getCurrentImageWidth()) * realZoom)) / 2;
	int zoomedYPos = (this->height() - static_cast<int>(static_cast<float>(this->getCurrentImageHeight()) * realZoom)) / 2;
	int zoomedWidth = static_cast<int>(static_cast<float>(this->image.width()) * realZoom);
	int zoomedHeight = static_cast<int>(static_cast<float>(this->image.height()) * realZoom);

	QRect sourceRect(0, 0, this->image.width(), this->image.height());

	const auto imageFlags = this->alphaEnabled
	                        ? Qt::ImageConversionFlag::AutoColor | Qt::ImageConversionFlag::NoAlpha
	                        : Qt::ImageConversionFlag::AutoColor;

	if (this->tileEnabled) {
		for (int i = -zoomedWidth; i <= zoomedWidth; i += zoomedWidth) {
			for (int j = -zoomedHeight; j <= zoomedHeight; j += zoomedHeight) {
				painter.drawImage(QRect(zoomedXPos + i, zoomedYPos + j, zoomedWidth, zoomedHeight), this->image, sourceRect, imageFlags);
			}
		}
	} else {
		painter.drawImage(QRect(zoomedXPos, zoomedYPos, zoomedWidth, zoomedHeight), this->image, sourceRect, imageFlags);
	}
}

void SVGWidget::setData(const std::vector<std::byte>& data) {
	QSvgRenderer renderer;
	renderer.load(QByteArray{reinterpret_cast<const char*>(data.data()), static_cast<qsizetype>(data.size())});
	auto size = renderer.defaultSize();
	this->image = QImage(size.width(), size.height(), QImage::Format_RGBA8888);
	this->image.fill(0);
	QPainter painter(&image);
	renderer.render(&painter);
	this->zoom = 1.f;
}

void VTFWidget::setData(const std::vector<std::byte>& data) {
	this->vtf = std::make_unique<VTF>(data);
	this->decodeImage(0, 0, 0, 0, this->alphaEnabled);
	this->zoom = 1.f;
}

bool VTFWidget::hasAlpha() const {
	return ImageFormatDetails::transparent(this->vtf->getFormat());
}

QString VTFWidget::getVersion() const {
	return QString::number(this->vtf->getMajorVersion()) + "." + QString::number(this->vtf->getMinorVersion());
}

QString VTFWidget::getFormat() const {
	return ::vtfFormatToString(this->vtf->getFormat());
}

int VTFWidget::getAuxCompression() const {
	if (this->vtf->getMajorVersion() < 7 || this->vtf->getMinorVersion() < 6) {
		return 0;
	}

	auto auxResource = this->vtf->getResource(Resource::TYPE_AUX_COMPRESSION);
	if (!auxResource) {
		return 0;
	}
	return auxResource->getDataAsAuxCompressionLevel();
}

void VTFWidget::paintEvent(QPaintEvent*) {
	QPainter painter(this);

	if (!this->vtf) {
		return;
	}

	float realZoom = static_cast<float>(1 << this->currentMip) * this->zoom;

	int zoomedXPos = (this->width() - static_cast<int>(static_cast<float>(this->getCurrentImageWidth()) * realZoom)) / 2;
	int zoomedYPos = (this->height() - static_cast<int>(static_cast<float>(this->getCurrentImageHeight()) * realZoom)) / 2;
	int zoomedWidth = static_cast<int>(static_cast<float>(this->image.width()) * realZoom);
	int zoomedHeight = static_cast<int>(static_cast<float>(this->image.height()) * realZoom);

	QRect sourceRect(0, 0, this->image.width(), this->image.height());

	if (this->showEverything && (this->getMaxFrame() > 1 || this->getMaxFace() > 1)) {
		int totalZoomedWidth = zoomedWidth * (this->getMaxFace() - 1);
		int totalZoomedHeight = zoomedHeight * (this->getMaxFrame() - 1);
		for (int face = 0; face < this->getMaxFace(); face++) {
			for (int frame = 0; frame < this->getMaxFrame(); frame++) {
				auto imageData = this->vtf->getImageDataAsRGBA8888(this->currentMip, frame, face, this->currentSlice);
				if (!imageData.empty()) {
					QImage currentImage(reinterpret_cast<uchar*>(imageData.data()), static_cast<int>(this->getCurrentImageWidth()), static_cast<int>(this->getCurrentImageHeight()), QImage::Format_RGBA8888);
					if (!this->alphaEnabled) {
						currentImage = currentImage.convertedTo(QImage::Format_RGB888);
					}
					painter.drawImage(QRect(zoomedXPos + (zoomedWidth * face) - (totalZoomedWidth / 2), zoomedYPos + (zoomedHeight * frame) - (totalZoomedHeight / 2), zoomedWidth, zoomedHeight), currentImage, sourceRect);
				}
			}
		}
	} else if (this->tileEnabled) {
		for (int i = -zoomedWidth; i <= zoomedWidth; i += zoomedWidth) {
			for (int j = -zoomedHeight; j <= zoomedHeight; j += zoomedHeight) {
				painter.drawImage(QRect(zoomedXPos + i, zoomedYPos + j, zoomedWidth, zoomedHeight), this->image, sourceRect);
			}
		}
	} else {
		painter.drawImage(QRect(zoomedXPos, zoomedYPos, zoomedWidth, zoomedHeight), this->image, sourceRect);
	}
}

void VTFWidget::decodeImage(int mip, int frame, int face, int slice, bool alpha) {
	if (!this->vtf) {
		return;
	}

	this->currentMip = mip;
	this->currentFrame = frame;
	this->currentFace = face;
	this->currentSlice = slice;
	this->alphaEnabled = alpha;

	this->imageData = this->vtf->getImageDataAsRGBA8888(this->currentMip, this->currentFrame, this->currentFace, this->currentSlice);
	if (this->imageData.empty()) {
		this->image = QImage();
		return;
	}
	this->image = QImage(reinterpret_cast<uchar*>(this->imageData.data()), static_cast<int>(this->vtf->getWidth(this->currentMip)), static_cast<int>(this->vtf->getHeight(this->currentMip)), QImage::Format_RGBA8888);
	if (!this->alphaEnabled) {
		this->image = this->image.convertedTo(QImage::Format_RGB888);
	}
}

TexturePreview::TexturePreview(QWidget* parent)
		: QWidget(parent) {
	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);

	this->image = new ImageWidget(this);
	layout->addWidget(this->image);

	this->svg = new SVGWidget(this);
	layout->addWidget(this->svg);

	this->vtf = new VTFWidget(this);
	layout->addWidget(this->vtf);

	auto* controls = new QWidget(this);
	controls->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
	layout->addWidget(controls);

	auto* controlsLayout = new QVBoxLayout(controls);

	auto* showEverythingCheckBoxParent = new QWidget(controls);
	auto* showEverythingCheckBoxLayout = new QHBoxLayout(showEverythingCheckBoxParent);
	auto* showEverythingCheckBoxLabel = new QLabel(tr("Lay Flat"), showEverythingCheckBoxParent);
	showEverythingCheckBoxLayout->addWidget(showEverythingCheckBoxLabel);
	this->showEverythingCheckBox = new QCheckBox(controls);
	QObject::connect(this->showEverythingCheckBox, &QCheckBox::stateChanged, this, [&] {
		this->vtf->setShowEverythingEnabled(this->showEverythingCheckBox->isChecked());
		this->frameSpin->setDisabled(this->vtf->getMaxFrame() == 1 || this->showEverythingCheckBox->isChecked());
		this->faceSpin->setDisabled(this->vtf->getMaxFace() == 1 || this->showEverythingCheckBox->isChecked());
		this->tileCheckBox->setDisabled(this->showEverythingCheckBox->isChecked());
		this->vtf->repaint();
	});
	showEverythingCheckBoxLayout->addWidget(this->showEverythingCheckBox, 0, Qt::AlignHCenter);
	controlsLayout->addWidget(showEverythingCheckBoxParent);

	auto* mipSpinParent = new QWidget(controls);
	auto* mipSpinLayout = new QHBoxLayout(mipSpinParent);
	auto* mipSpinLabel = new QLabel(tr("Mip"), mipSpinParent);
	mipSpinLayout->addWidget(mipSpinLabel);
	this->mipSpin = new QSpinBox(controls);
	this->mipSpin->setMinimum(0);
	QObject::connect(this->mipSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [&] {
		this->vtf->setMip(this->mipSpin->value());
		this->vtf->repaint();
		this->sizeLabel->setText(QString("%1x%2").arg(this->vtf->getCurrentImageWidth()).arg(this->vtf->getCurrentImageHeight()));
	});
	mipSpinLayout->addWidget(this->mipSpin);
	controlsLayout->addWidget(mipSpinParent);

	auto* frameSpinParent = new QWidget(controls);
	auto* frameSpinLayout = new QHBoxLayout(frameSpinParent);
	auto* frameSpinLabel = new QLabel(tr("Frame"), frameSpinParent);
	frameSpinLayout->addWidget(frameSpinLabel);
	this->frameSpin = new QSpinBox(frameSpinParent);
	this->frameSpin->setMinimum(0);
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
	this->faceSpin->setMinimum(0);
	QObject::connect(this->faceSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [&] {
		this->vtf->setFace(this->faceSpin->value());
		this->vtf->repaint();
	});
	faceSpinLayout->addWidget(this->faceSpin);
	controlsLayout->addWidget(faceSpinParent);

	auto* sliceSpinParent = new QWidget(controls);
	auto* sliceSpinLayout = new QHBoxLayout(sliceSpinParent);
	auto* sliceSpinLabel = new QLabel(tr("Slice"), sliceSpinParent);
	sliceSpinLayout->addWidget(sliceSpinLabel);
	this->sliceSpin = new QSpinBox(controls);
	this->sliceSpin->setMinimum(0);
	QObject::connect(this->sliceSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [&] {
		this->vtf->setSlice(this->sliceSpin->value());
		this->vtf->repaint();
	});
	sliceSpinLayout->addWidget(this->sliceSpin);
	controlsLayout->addWidget(sliceSpinParent);

	auto* alphaCheckBoxParent = new QWidget(controls);
	auto* alphaCheckBoxLayout = new QHBoxLayout(alphaCheckBoxParent);
	auto* alphaCheckBoxLabel = new QLabel(tr("Alpha"), alphaCheckBoxParent);
	alphaCheckBoxLayout->addWidget(alphaCheckBoxLabel);
	this->alphaCheckBox = new QCheckBox(controls);
	QObject::connect(this->alphaCheckBox, &QCheckBox::stateChanged, this, [&] {
		this->image->setAlphaEnabled(this->alphaCheckBox->isChecked());
		this->svg->setAlphaEnabled(this->alphaCheckBox->isChecked());
		this->vtf->setAlphaEnabled(this->alphaCheckBox->isChecked());
		this->getVisibleWidget()->repaint();
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
		this->svg->setTileEnabled(this->tileCheckBox->isChecked());
		this->vtf->setTileEnabled(this->tileCheckBox->isChecked());
		this->getVisibleWidget()->repaint();
	});
	tileCheckBoxLayout->addWidget(this->tileCheckBox, 0, Qt::AlignHCenter);
	controlsLayout->addWidget(tileCheckBoxParent);

	auto* zoomSliderParent = new QWidget(controls);
	auto* zoomSliderLayout = new QHBoxLayout(zoomSliderParent);
	auto* zoomSliderLabel = new QLabel(tr("Zoom"), zoomSliderParent);
	zoomSliderLayout->addWidget(zoomSliderLabel);
	this->zoomSlider = new QSlider(controls);
	this->zoomSlider->setMinimum(10);
	this->zoomSlider->setMaximum(1000);
	this->zoomSlider->setValue(100);
	QObject::connect(this->zoomSlider, &QSlider::valueChanged, this, [&] {
		this->image->setZoom(this->zoomSlider->value());
		this->svg->setZoom(this->zoomSlider->value());
		this->vtf->setZoom(this->zoomSlider->value());
		this->getVisibleWidget()->repaint();
	});
	zoomSliderLayout->addWidget(this->zoomSlider, 0, Qt::AlignHCenter);
	controlsLayout->addWidget(zoomSliderParent);

	this->sizeLabel = new QLabel(controls);
	this->sizeLabel->setAlignment(Qt::AlignRight);
	controlsLayout->addWidget(this->sizeLabel);

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

void TexturePreview::setImageData(const std::vector<std::byte>& data) const {
	this->image->show();
	this->svg->hide();
	this->vtf->hide();

	this->image->setData(data);
	this->setData(this->image);
}

void TexturePreview::setSVGData(const std::vector<std::byte>& data) const {
	this->image->hide();
	this->svg->show();
	this->vtf->hide();

	this->svg->setData(data);
	this->setData(this->svg);
}

void TexturePreview::setVTFData(const std::vector<std::byte>& data) const {
	this->image->hide();
	this->svg->hide();
	this->vtf->show();

	this->vtf->setData(data);
	this->setData(this->vtf);
}

void TexturePreview::setData(ITextureWidget* widget) const {
	this->showEverythingCheckBox->setDisabled(widget->getMaxFrame() == 1 && widget->getMaxFace() == 1);

	this->mipSpin->setMaximum(widget->getMaxMip() - 1);
	this->mipSpin->setValue(0);
	this->mipSpin->setDisabled(widget->getMaxMip() == 1);

	this->frameSpin->setMaximum(widget->getMaxFrame() - 1);
	this->frameSpin->setValue(0);
	this->frameSpin->setDisabled(widget->getMaxFrame() == 1 || widget->getShowEverythingEnabled());

	this->faceSpin->setMaximum(widget->getMaxFace() - 1);
	this->faceSpin->setValue(0);
	this->faceSpin->setDisabled(widget->getMaxFace() == 1 || widget->getShowEverythingEnabled());

	this->sliceSpin->setMaximum(widget->getMaxSlice() - 1);
	this->sliceSpin->setValue(0);
	this->sliceSpin->setDisabled(widget->getMaxSlice() == 1);

	// Don't reset alpha: this is handled automatically
	//this->alphaCheckBox->setChecked(false);
	this->alphaCheckBox->setDisabled(!widget->hasAlpha());

	// Don't reset tiled: this is handled automatically
	//this->tileCheckBox->setChecked(false);
	this->tileCheckBox->setDisabled(widget->getShowEverythingEnabled());

	// Don't reset zoom: set the preexisting zoom on the vtf
	//this->zoomSlider->setValue(100);
	widget->setZoom(this->zoomSlider->value());

	this->sizeLabel->setText(QString("%1x%2").arg(widget->getCurrentImageWidth()).arg(widget->getCurrentImageHeight()));

	this->versionLabel->setText(tr("Version: %1").arg(widget->getVersion()));

	this->imageFormatLabel->setText(tr("Format: %1").arg(widget->getFormat()));

	this->compressionLevelLabel->setText(tr("Compression: %1").arg(widget->getAuxCompression()));
	this->compressionLevelLabel->setVisible(widget->getAuxCompression() != 0);
}

ITextureWidget* TexturePreview::getVisibleWidget() const {
	if (this->image->isVisible()) {
		return this->image;
	}
	if (this->svg->isVisible()) {
		return this->svg;
	}
	return this->vtf;
}

void TexturePreview::wheelEvent(QWheelEvent* event) {
	if (QPoint numDegrees = event->angleDelta() / 8; !numDegrees.isNull()) {
		this->zoomSlider->setValue(this->zoomSlider->value() + numDegrees.y());
	}
	event->accept();
}
