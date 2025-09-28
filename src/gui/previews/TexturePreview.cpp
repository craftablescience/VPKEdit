#include "TexturePreview.h"

#include <cmath>
#include <cstdint>
#include <numbers>
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

#include "../utility/ImageLoader.h"
#include "../FileViewer.h"

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
		{ RG1616F, "RG1616F" },
		{ RG3232F, "RG3232F" },
		{ RGBX8888, "RGBX8888" },
		{ EMPTY, "EMPTY" },
		{ ATI2N, "ATI2N" },
		{ ATI1N, "ATI1N" },
		{ RGBA1010102, "RGBA1010102" },
		{ BGRA1010102, "BGRA1010102" },
		{ R16F, "R16F" },
		{ CONSOLE_BGRX8888_LINEAR, "CONSOLE_BGRX8888_LINEAR" },
		{ CONSOLE_RGBA8888_LINEAR, "CONSOLE_RGBA8888_LINEAR" },
		{ CONSOLE_ABGR8888_LINEAR, "CONSOLE_ABGR8888_LINEAR" },
		{ CONSOLE_ARGB8888_LINEAR, "CONSOLE_ARGB8888_LINEAR" },
		{ CONSOLE_BGRA8888_LINEAR, "CONSOLE_BGRA8888_LINEAR" },
		{ CONSOLE_RGB888_LINEAR, "CONSOLE_RGB888_LINEAR" },
		{ CONSOLE_BGR888_LINEAR, "CONSOLE_BGR888_LINEAR" },
		{ CONSOLE_BGRX5551_LINEAR, "CONSOLE_BGRX5551_LINEAR" },
		{ CONSOLE_I8_LINEAR, "CONSOLE_I8_LINEAR" },
		{ CONSOLE_RGBA16161616_LINEAR, "CONSOLE_RGBA16161616_LINEAR" },
		{ CONSOLE_BGRX8888_LE, "CONSOLE_BGRX8888_LE" },
		{ CONSOLE_BGRA8888_LE, "CONSOLE_BGRA8888_LE" },
		{ R8, "R8" },
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

	auto* contextMenu = new QMenu{this};

	contextMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("&Copy Image"), [this] {
		QApplication::clipboard()->setImage(this->image, QClipboard::Clipboard);
	});

	contextMenu->addSeparator();

	contextMenu->addAction(QIcon::fromTheme("zoom-in"), tr("Zoom &In"), [this] {
		this->setZoom(this->getZoom() + 150.f, true);
		this->update();
	});

	contextMenu->addAction(QIcon::fromTheme("zoom-out"), tr("Zoom &Out"), [this] {
		this->setZoom(this->getZoom() - 150.f, true);
		this->update();
	});

	contextMenu->addAction(QIcon::fromTheme("zoom-original"), tr("Reset &Zoom"), [this] {
		this->setZoom(DEFAULT_ZOOM, true);
		this->update();
	});

	contextMenu->addAction(QIcon::fromTheme("view-restore"), tr("Reset &Pan"), [this] {
		this->setOffset({});
		this->update();
	});

	QObject::connect(this, &VTFWidget::customContextMenuRequested, this, [this, contextMenu](const QPoint& pos) {
		if (!this->image.isNull()) {
			(void) contextMenu->exec(this->mapToGlobal(pos));
		}
	});
}

void ITextureWidget::setZoom(int zoom_, bool emitSignal) {
	const float clamped = std::clamp(static_cast<float>(zoom_), MIN_ZOOM, MAX_ZOOM);
	this->zoom = clamped / 100.f;
	if (emitSignal) {
		emit this->zoomUpdated(clamped);
	}
}

float ITextureWidget::getScaledZoom() const {
	// Gives a more natural scaling curve
	return 10.f * std::pow((this->zoom + 5.f) / 15.f, std::numbers::e_v<float>);
}

void ImageWidget::setData(const std::vector<std::byte>& data) {
	this->image = ImageLoader::load(data);
	this->zoom = 1.f;
}

QString ImageWidget::getFormat() const {
	// Add 7 for the length of "Format_"
	return QMetaEnum::fromType<QImage::Format>().valueToKey(this->image.format()) + 7;
}

void ImageWidget::paintEvent(QPaintEvent* /*event*/) {
	QPainter painter(this);

	const float realZoom = static_cast<float>(1 << this->currentMip) * this->getScaledZoom();

	const int zoomedXPos = this->offset.x() + ((this->width()  - static_cast<int>(static_cast<float>(this->getCurrentImageWidth())  * realZoom)) / 2);
	const int zoomedYPos = this->offset.y() + ((this->height() - static_cast<int>(static_cast<float>(this->getCurrentImageHeight()) * realZoom)) / 2);
	const int zoomedWidth =  static_cast<int>(static_cast<float>(this->image.width())  * realZoom);
	const int zoomedHeight = static_cast<int>(static_cast<float>(this->image.height()) * realZoom);

	const QRect sourceRect(0, 0, this->image.width(), this->image.height());

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
	const auto size = renderer.defaultSize();
	this->image = QImage(size.width(), size.height(), QImage::Format_RGBA8888);
	this->image.fill(0);
	QPainter painter(&image);
	renderer.render(&painter);
	this->zoom = 1.f;
}

void PPLWidget::setData(const std::vector<std::byte>& data) {
	this->ppl = std::make_unique<PPL>(data);
	if (this->ppl) {
		this->decodeImage();
	}
	this->zoom = 1.f;
}

bool PPLWidget::hasAlpha() const {
	return false;
}

QString PPLWidget::getVersion() const {
	return QString::number(this->ppl->getVersion());
}

QString PPLWidget::getFormat() const {
	return ::vtfFormatToString(this->ppl->getFormat());
}

void PPLWidget::paintEvent(QPaintEvent*) {
	QPainter painter(this);

	if (!this->ppl) {
		return;
	}

	const float realZoom = static_cast<float>(1 << this->currentMip) * this->getScaledZoom();

	const int zoomedXPos = this->offset.x() + ((this->width()  - static_cast<int>(static_cast<float>(this->getCurrentImageWidth())  * realZoom)) / 2);
	const int zoomedYPos = this->offset.y() + ((this->height() - static_cast<int>(static_cast<float>(this->getCurrentImageHeight()) * realZoom)) / 2);
	const int zoomedWidth =  static_cast<int>(static_cast<float>(this->image.width())  * realZoom);
	const int zoomedHeight = static_cast<int>(static_cast<float>(this->image.height()) * realZoom);

	const QRect sourceRect(0, 0, this->image.width(), this->image.height());

	if (this->showEverything && (this->getMaxFrame() > 1 || this->getMaxFace() > 1)) {
		const int totalZoomedWidth = zoomedWidth * (this->getMaxFace() - 1);
		const int totalZoomedHeight = zoomedHeight * (this->getMaxFrame() - 1);
		for (int face = 0; face < this->getMaxFace(); face++) {
			for (int frame = 0; frame < this->getMaxFrame(); frame++) {
				if (auto imageData = this->ppl->getImageAsRGB888(this->currentFrame)) {
					QImage currentImage(reinterpret_cast<uchar*>(imageData->data.data()), this->getCurrentImageWidth(), this->getCurrentImageHeight(), QImage::Format_RGB888);
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

void PPLWidget::decodeImage() {
	if (!this->ppl) {
		return;
	}

	uint32_t lowestLOD = UINT32_MAX;
	for (auto lod : this->ppl->getImageLODs()) {
		if (lod < lowestLOD) {
			lowestLOD = lod;
		}
	}
	auto image_ = this->ppl->getImageAsRGB888(lowestLOD);
	if (!image_) {
		this->image = QImage();
		return;
	}
	this->imageData = image_->data;
	if (this->imageData.empty()) {
		this->image = QImage();
		return;
	}
	this->image = QImage(reinterpret_cast<uchar*>(this->imageData.data()), static_cast<int>(image_->width), static_cast<int>(image_->height), QImage::Format_RGB888);
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
	switch (this->vtf->getPlatform()) {
		case VTF::PLATFORM_UNKNOWN:
		case VTF::PLATFORM_PC:
			return "7." + QString::number(this->vtf->getVersion());
		case VTF::PLATFORM_XBOX:
			return "XBOX (v7.2)";
		case VTF::PLATFORM_PS3_PORTAL2:
			return "PS3 (v7.5)";
		case VTF::PLATFORM_PS3_ORANGEBOX:
			return "PS3 (v7.4)";
		case VTF::PLATFORM_X360:
			return "X360 (v7.4)";
	}
	return "";
}

QString VTFWidget::getFormat() const {
	return ::vtfFormatToString(this->vtf->getFormat());
}

int VTFWidget::getAuxCompression() const {
	if (this->vtf->getVersion() < 6) {
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

	const float realZoom = static_cast<float>(1 << this->currentMip) * this->getScaledZoom();

	const int zoomedXPos = this->offset.x() + ((this->width()  - static_cast<int>(static_cast<float>(this->getCurrentImageWidth())  * realZoom)) / 2);
	const int zoomedYPos = this->offset.y() + ((this->height() - static_cast<int>(static_cast<float>(this->getCurrentImageHeight()) * realZoom)) / 2);
	const int zoomedWidth =  static_cast<int>(static_cast<float>(this->image.width())  * realZoom);
	const int zoomedHeight = static_cast<int>(static_cast<float>(this->image.height()) * realZoom);

	const QRect sourceRect(0, 0, this->image.width(), this->image.height());

	if (this->showEverything && (this->getMaxFrame() > 1 || this->getMaxFace() > 1)) {
		const int totalZoomedWidth = zoomedWidth * (this->getMaxFace() - 1);
		const int totalZoomedHeight = zoomedHeight * (this->getMaxFrame() - 1);
		for (int face = 0; face < this->getMaxFace(); face++) {
			for (int frame = 0; frame < this->getMaxFrame(); frame++) {
				auto imageData = this->vtf->getImageDataAsRGBA8888(this->currentMip, frame, face, this->currentSlice);
				if (!imageData.empty()) {
					QImage currentImage(reinterpret_cast<uchar*>(imageData.data()), this->getCurrentImageWidth(), this->getCurrentImageHeight(), QImage::Format_RGBA8888);
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

TexturePreview::TexturePreview(QWidget* parent, FileViewer* fileViewer_)
		: QWidget(parent)
		, fileViewer(fileViewer_) {
	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);

	this->image = new ImageWidget(this);
	layout->addWidget(this->image);

	this->svg = new SVGWidget(this);
	layout->addWidget(this->svg);

	this->ppl = new PPLWidget(this);
	layout->addWidget(this->ppl);

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
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	QObject::connect(this->showEverythingCheckBox, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState) {
#else
	QObject::connect(this->showEverythingCheckBox, &QCheckBox::stateChanged, this, [&](int) {
#endif
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	QObject::connect(this->alphaCheckBox, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState) {
#else
	QObject::connect(this->alphaCheckBox, &QCheckBox::stateChanged, this, [&](int) {
#endif
		this->image->setAlphaEnabled(this->alphaCheckBox->isChecked());
		this->svg->setAlphaEnabled(this->alphaCheckBox->isChecked());
		this->ppl->setAlphaEnabled(this->alphaCheckBox->isChecked());
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	QObject::connect(this->tileCheckBox, &QCheckBox::checkStateChanged, this, [&](Qt::CheckState) {
#else
	QObject::connect(this->tileCheckBox, &QCheckBox::stateChanged, this, [&](int) {
#endif
		this->image->setTileEnabled(this->tileCheckBox->isChecked());
		this->svg->setTileEnabled(this->tileCheckBox->isChecked());
		this->ppl->setTileEnabled(this->tileCheckBox->isChecked());
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
	this->zoomSlider->setMinimum(ITextureWidget::MIN_ZOOM);
	this->zoomSlider->setMaximum(ITextureWidget::MAX_ZOOM);
	this->zoomSlider->setValue(ITextureWidget::DEFAULT_ZOOM);
	QObject::connect(this->zoomSlider, &QSlider::valueChanged, this, [this] {
		this->image->setZoom(this->zoomSlider->value());
		this->svg->setZoom(this->zoomSlider->value());
		this->ppl->setZoom(this->zoomSlider->value());
		this->vtf->setZoom(this->zoomSlider->value());
		this->getVisibleWidget()->repaint();
	});
	for (const auto* widget : {this->image, this->svg, this->ppl, this->vtf}) {
		QObject::connect(widget, &ITextureWidget::zoomUpdated, this, [this](float zoom) {
			this->zoomSlider->setValue(zoom);
		});
	}
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
	this->ppl->hide();
	this->vtf->hide();

	this->image->setData(data);
	this->setData(this->image);
}

void TexturePreview::setSVGData(const std::vector<std::byte>& data) const {
	this->image->hide();
	this->svg->show();
	this->ppl->hide();
	this->vtf->hide();

	this->svg->setData(data);
	this->setData(this->svg);
}

void TexturePreview::setPPLData(const std::vector<std::byte>& data) const {
	this->image->hide();
	this->svg->hide();
	this->ppl->show();
	this->vtf->hide();

	this->ppl->setData(data);
	this->setData(this->ppl);
}

void TexturePreview::setTTXData(const std::vector<std::byte>& tthData, const std::vector<std::byte>& ttzData) const {
	this->image->hide();
	this->svg->hide();
	this->ppl->hide();
	this->vtf->show();

	TTX ttx{tthData, ttzData};
	if (!ttx) {
		this->fileViewer->showFileLoadErrorPreview();
	}

	this->vtf->setData(ttx.getVTF().bake());
	this->setData(this->vtf);
}

void TexturePreview::setVTFData(const std::vector<std::byte>& data) const {
	this->image->hide();
	this->svg->hide();
	this->ppl->hide();
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

	this->sliceSpin->setMaximum(widget->getCurrentImageDepth() - 1);
	this->sliceSpin->setValue(0);
	this->sliceSpin->setDisabled(widget->getCurrentImageDepth() == 1);

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
	if (this->ppl->isVisible()) {
		return this->ppl;
	}
	return this->vtf;
}

void TexturePreview::mouseMoveEvent(QMouseEvent* event) {
	if (this->dragging) {
		const auto delta = event->pos() - this->lastDrag;
		auto* widget = this->getVisibleWidget();
		widget->setOffset(widget->getOffset() + delta);
		widget->repaint();
		this->lastDrag = event->pos();
	}
	event->accept();
}

void TexturePreview::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::MouseButton::LeftButton || event->button() == Qt::MouseButton::MiddleButton) {
		this->setCursor({Qt::CursorShape::ClosedHandCursor});
		this->lastDrag = event->pos();
		this->dragging = true;
	}
	event->accept();
}

void TexturePreview::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::MouseButton::LeftButton || event->button() == Qt::MouseButton::MiddleButton) {
		this->setCursor({Qt::CursorShape::ArrowCursor});
		this->dragging = false;
	}
	event->accept();
}

void TexturePreview::wheelEvent(QWheelEvent* event) {
	if (const QPoint numDegrees = event->angleDelta() / 8; !numDegrees.isNull()) {
		const float mult = event->modifiers() & Qt::Modifier::CTRL ? 10.f : 1.f;
		this->zoomSlider->setValue(this->zoomSlider->value() + numDegrees.y() * mult);
	}
	event->accept();
}
