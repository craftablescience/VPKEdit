// ReSharper disable CppUnusedIncludeDirective

#include "TexturePreview.hpp"

#include <cmath>
#include <stdexcept>

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QStyle>
#include <QWheelEvent>
#include <vtfpp/ImagePixel.h>

namespace {

[[nodiscard]] std::vector<std::byte> drawCubemapNet(uint16_t faceWidth, uint16_t faceHeight, const QImage& xp, const QImage& xn, const QImage& yp, const QImage& yn, const QImage& zp, const QImage& zn, const QImage& sm) {
	QImage net{faceWidth * 4, faceHeight * 3, QImage::Format_ARGB32};
	net.fill(Qt::transparent);

	QPainter painter{&net};
	painter.drawImage(faceWidth * 1, faceHeight * 0, yp);
	painter.drawImage(faceWidth * 0, faceHeight * 1, xn);
	painter.drawImage(faceWidth * 1, faceHeight * 1, zp);
	painter.drawImage(faceWidth * 2, faceHeight * 1, xp);
	painter.drawImage(faceWidth * 3, faceHeight * 1, zn);
	painter.drawImage(faceWidth * 1, faceHeight * 2, yn);
	if (!sm.isNull()) {
		painter.drawImage(faceWidth * 2, faceHeight * 2, sm);
	}
	painter.end();

	return {reinterpret_cast<const std::byte*>(net.constBits()), reinterpret_cast<const std::byte*>(net.constBits()) + net.sizeInBytes()};
}

} // namespace

MareTF::QMareTextureWidget::QMareTextureWidget(QWidget* parent) : QWidget{parent} {
	this->setContextMenuPolicy(Qt::CustomContextMenu);

	auto* contextMenu = new QMenu{this};

	contextMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("&Copy Image"), [this] {
		QApplication::clipboard()->setImage(this->textureCurrent, QClipboard::Clipboard);
	});

	contextMenu->addSeparator();

	contextMenu->addAction(QIcon::fromTheme("zoom-in"), tr("Zoom &In"), [this] {
		this->applyIncrementalZoom(1.f);
		this->update();
	});

	contextMenu->addAction(QIcon::fromTheme("zoom-out"), tr("Zoom &Out"), [this] {
		this->applyIncrementalZoom(-1.f);
		this->update();
	});

	contextMenu->addAction(QIcon::fromTheme("zoom-original"), tr("Reset &Zoom"), [this] {
		this->textureZoom = 1.f;
		this->update();
	});

	contextMenu->addAction(QIcon::fromTheme("view-restore"), tr("Reset &Pan"), [this] {
		this->textureOffset = {};
		this->update();
	});

	QObject::connect(this, &QMareTextureWidget::customContextMenuRequested, this, [this, contextMenu](const QPoint& pos) {
		if (!this->textureCurrent.isNull()) {
			(void) contextMenu->exec(this->mapToGlobal(pos));
		}
	});
}

bool MareTF::QMareTextureWidget::loadImage(const quint8* dataPtr, quint64 dataLen) {
	try {
		Image tmpImage;
		tmpImage.imageData = vtfpp::ImageConversion::convertFileToImageData({reinterpret_cast<const std::byte*>(dataPtr), dataLen}, tmpImage.format, tmpImage.width, tmpImage.height, tmpImage.frameCount);
		this->textureFile = std::move(tmpImage);
		this->reloadCurrentTexture();
	} catch (const std::overflow_error&) {
		return false;
	}
	return true;
}

bool MareTF::QMareTextureWidget::loadPPL(const quint8* dataPtr, quint64 dataLen) {
	try {
		auto tmpPPL = vtfpp::PPL{{reinterpret_cast<const std::byte*>(dataPtr), dataLen}};
		if (!tmpPPL || tmpPPL.getImageLODs().empty()) {
			return false;
		}
		this->textureFile = std::move(tmpPPL);
		this->reloadCurrentTexture();
	} catch (const std::overflow_error&) {
		return false;
	}
	return true;
}

bool MareTF::QMareTextureWidget::loadVTF(const quint8* dataPtr, quint64 dataLen) {
	try {
		auto tmpVTF = vtfpp::VTF{{reinterpret_cast<const std::byte*>(dataPtr), dataLen}};
		if (!tmpVTF) {
			return false;
		}
		if (
			tmpVTF.getFaceCount() >= 6 ||
			tmpVTF.getFlags() & vtfpp::VTF::FLAG_V0_NORMAL ||
			(tmpVTF.getVersion() >= 3 && tmpVTF.getFlags() & vtfpp::VTF::FLAG_V3_SSBUMP)
		) {
			this->a = false;
		}
		this->textureFile = std::move(tmpVTF);
		this->reloadCurrentTexture();
	} catch (const std::overflow_error&) {
		return false;
	}
	return true;
}

void MareTF::QMareTextureWidget::reloadCurrentTexture() {
	this->textureCurrent = QImage{};
	this->textureCurrentData.clear();

	vtfpp::ImageFormat format;
	QImage::Format formatQt;

	const auto processImageData = [this, &format, &formatQt] {
		if ((!this->r && vtfpp::ImageFormatDetails::red(format) > 0) || (!this->g && vtfpp::ImageFormatDetails::green(format) > 0) || (!this->b && vtfpp::ImageFormatDetails::blue(format) > 0)) {
			for (int y = 0; y < this->textureCurrent.height(); y++) {
				const auto processScanLine = [this, scanLineRaw = this->textureCurrent.scanLine(y)]<vtfpp::ImagePixel::PixelType P> {
					auto* scanLine = reinterpret_cast<P*>(scanLineRaw);
					for (int x = 0; x < this->textureCurrent.width(); x++) {
						if (!this->r) {
							scanLine[x].setR(0);
						}
						if (!this->g) {
							scanLine[x].setG(0);
						}
						if (!this->b) {
							scanLine[x].setB(0);
						}
					}
				};
				if (formatQt == QImage::Format_ARGB32_Premultiplied || formatQt == QImage::Format_ARGB32) {
					processScanLine.operator()<vtfpp::ImagePixel::BGRA8888>();
				} else /*if (formatQt == QImage::Format_RGB888)*/ {
					processScanLine.operator()<vtfpp::ImagePixel::RGB888>();
				}
			}
		}
	};

	if (std::holds_alternative<Image>(this->textureFile)) {
		const auto& image = std::get<Image>(this->textureFile);

		if (this->a) {
			if (!this->r && !this->g && !this->b && !this->aMask) {
				format = vtfpp::ImageFormat::A8;
				formatQt = QImage::Format_Grayscale8;
			} else {
				format = vtfpp::ImageFormat::BGRA8888;
				formatQt = vtfpp::ImageFormatDetails::decompressedAlpha(image.format) ? QImage::Format_ARGB32 : QImage::Format_ARGB32_Premultiplied;
			}
		} else {
			format = vtfpp::ImageFormat::RGB888;
			formatQt = QImage::Format_RGB888;
		}

		this->textureCurrentData = image.getImageDataAs(format, this->currentFrame);
		this->textureCurrent = {reinterpret_cast<const uchar*>(this->textureCurrentData.data()), image.width, image.height, formatQt};

		processImageData();
		this->update();
	} else if (std::holds_alternative<vtfpp::PPL>(this->textureFile) && std::get<vtfpp::PPL>(this->textureFile)) {
		const auto& ppl = std::get<vtfpp::PPL>(this->textureFile);

		if (this->a) {
			if (!this->r && !this->g && !this->b && !this->aMask) {
				format = vtfpp::ImageFormat::A8;
				formatQt = QImage::Format_Grayscale8;
			} else {
				format = vtfpp::ImageFormat::BGRA8888;
				formatQt = vtfpp::ImageFormatDetails::decompressedAlpha(ppl.getFormat()) ? QImage::Format_ARGB32 : QImage::Format_ARGB32_Premultiplied;
			}
		} else {
			format = vtfpp::ImageFormat::RGB888;
			formatQt = QImage::Format_RGB888;
		}

		const auto image = ppl.getImageAs(format, ppl.getImageLODs().at(this->currentFrame));
		if (!image) {
			return;
		}

		this->textureCurrentData = image->data;
		this->textureCurrent = {reinterpret_cast<const uchar*>(this->textureCurrentData.data()), static_cast<int>(image->width), static_cast<int>(image->height), formatQt};

		processImageData();
		this->update();
	} else if (std::holds_alternative<vtfpp::VTF>(this->textureFile) && std::get<vtfpp::VTF>(this->textureFile)) {
		const auto& vtf = std::get<vtfpp::VTF>(this->textureFile);

		if (this->a) {
			if (!this->r && !this->g && !this->b && !this->aMask) {
				format = vtfpp::ImageFormat::A8;
				formatQt = QImage::Format_Grayscale8;
			} else {
				format = vtfpp::ImageFormat::BGRA8888;
				formatQt = vtfpp::ImageFormatDetails::decompressedAlpha(vtf.getFormat()) ? QImage::Format_ARGB32 : QImage::Format_ARGB32_Premultiplied;
			}
		} else {
			format = vtfpp::ImageFormat::RGB888;
			formatQt = QImage::Format_RGB888;
		}

		const auto width = vtf.getWidth(this->currentMip);
		const auto height = vtf.getHeight(this->currentMip);
		if (vtf.getFaceCount() >= 6 && this->cubemapMode == 0) {
			const auto xpData = vtf.getImageDataAs(format, this->currentMip, this->currentFrame, 0, this->currentDepth);
			const auto xnData = vtf.getImageDataAs(format, this->currentMip, this->currentFrame, 1, this->currentDepth);
			const auto ypData = vtf.getImageDataAs(format, this->currentMip, this->currentFrame, 2, this->currentDepth);
			const auto ynData = vtf.getImageDataAs(format, this->currentMip, this->currentFrame, 3, this->currentDepth);
			const auto zpData = vtf.getImageDataAs(format, this->currentMip, this->currentFrame, 4, this->currentDepth);
			const auto znData = vtf.getImageDataAs(format, this->currentMip, this->currentFrame, 5, this->currentDepth);
			const auto smData = vtf.getFaceCount() > 6 ? vtf.getImageDataAs(format, this->currentMip, this->currentFrame, 6, this->currentDepth) : std::vector<std::byte>{};

			const QImage xp{reinterpret_cast<const uchar*>(xpData.data()), width, height, formatQt};
			const QImage xn{reinterpret_cast<const uchar*>(xnData.data()), width, height, formatQt};
			const QImage yp{reinterpret_cast<const uchar*>(ypData.data()), width, height, formatQt};
			const QImage yn{reinterpret_cast<const uchar*>(ynData.data()), width, height, formatQt};
			const QImage zp{reinterpret_cast<const uchar*>(zpData.data()), width, height, formatQt};
			const QImage zn{reinterpret_cast<const uchar*>(znData.data()), width, height, formatQt};
			const auto sm = !smData.empty() ? QImage{reinterpret_cast<const uchar*>(smData.data()), width, height, formatQt} : QImage{};

			this->textureCurrentData = ::drawCubemapNet(vtf.getWidth(this->currentMip), vtf.getHeight(this->currentMip), xp, xn, yp, yn, zp, zn, sm);
			this->textureCurrent = {reinterpret_cast<const uchar*>(this->textureCurrentData.data()), width * 4, height * 3, formatQt = QImage::Format_ARGB32};
		} else {
			this->textureCurrentData = vtf.getImageDataAs(format, this->currentMip, this->currentFrame, this->currentFace, this->currentDepth);
			this->textureCurrent = {reinterpret_cast<const uchar*>(this->textureCurrentData.data()), width, height, formatQt};
		}

		processImageData();
		this->update();
	}
}

void MareTF::QMareTextureWidget::applyIncrementalZoom(float increment) {
	this->textureZoom *= std::exp(increment * 0.25f);
	this->textureZoom = qMax(0.25f, this->textureZoom);
	this->update();
}

uint8_t MareTF::QMareTextureWidget::getCurrentMip() const {
	return this->currentMip;
}

void MareTF::QMareTextureWidget::setCurrentMip(uint8_t mip) {
	if (std::holds_alternative<vtfpp::VTF>(this->textureFile)) {
		this->currentMip = std::clamp<uint8_t>(mip, 0, std::get<vtfpp::VTF>(this->textureFile).getMipCount() - 1);
		this->reloadCurrentTexture();
	}
}

uint16_t MareTF::QMareTextureWidget::getCurrentFrame() const {
	return this->currentFrame;
}

void MareTF::QMareTextureWidget::setCurrentFrame(uint16_t frame) {
	if (std::holds_alternative<vtfpp::PPL>(this->textureFile)) {
		this->currentFrame = std::clamp<uint16_t>(frame, 0, std::get<vtfpp::PPL>(this->textureFile).getImageLODs().size() - 1);
		this->reloadCurrentTexture();
	} else if (std::holds_alternative<vtfpp::VTF>(this->textureFile)) {
		this->currentFrame = std::clamp<uint16_t>(frame, 0, std::get<vtfpp::VTF>(this->textureFile).getFrameCount() - 1);
		this->reloadCurrentTexture();
	}
}

uint8_t MareTF::QMareTextureWidget::getCurrentFace() const {
	return this->currentFace;
}

void MareTF::QMareTextureWidget::setCurrentFace(uint8_t face) {
	if (std::holds_alternative<vtfpp::VTF>(this->textureFile)) {
		this->currentFace = std::clamp<uint8_t>(face, 0, std::get<vtfpp::VTF>(this->textureFile).getFaceCount() - 1);
		this->reloadCurrentTexture();
	}
}

uint16_t MareTF::QMareTextureWidget::getCurrentDepth() const {
	return this->currentDepth;
}

void MareTF::QMareTextureWidget::setCurrentDepth(uint16_t depth) {
	if (std::holds_alternative<vtfpp::VTF>(this->textureFile)) {
		this->currentDepth = std::clamp<uint16_t>(depth, 0, std::get<vtfpp::VTF>(this->textureFile).getDepth() - 1);
		this->reloadCurrentTexture();
	}
}

bool MareTF::QMareTextureWidget::useR() const {
	return this->r;
}

bool MareTF::QMareTextureWidget::useG() const {
	return this->g;
}

bool MareTF::QMareTextureWidget::useB() const {
	return this->b;
}

void MareTF::QMareTextureWidget::setRGB(bool newR, bool newG, bool newB) {
	this->r = newR;
	this->g = newG;
	this->b = newB;
	this->reloadCurrentTexture();
}

bool MareTF::QMareTextureWidget::useA() const {
	return this->a;
}

void MareTF::QMareTextureWidget::setA(bool newA) {
	this->a = newA;
	this->reloadCurrentTexture();
}

bool MareTF::QMareTextureWidget::useAMask() const {
	return this->aMask;
}

void MareTF::QMareTextureWidget::setAMask(bool newAMask) {
	this->aMask = newAMask;
	this->reloadCurrentTexture();
}

bool MareTF::QMareTextureWidget::useTiled() const {
	return this->tiled;
}

void MareTF::QMareTextureWidget::setTiled(bool newTiled) {
	this->tiled = newTiled;
	this->reloadCurrentTexture();
}

int MareTF::QMareTextureWidget::getCurrentCubemapMode() const {
	return this->cubemapMode;
}

void MareTF::QMareTextureWidget::setCurrentCubemapMode(int mode) {
	this->cubemapMode = mode;
	this->reloadCurrentTexture();
}

MareTF::QMareTextureWidget::operator bool() const {
	return !this->textureCurrent.isNull();
}

void MareTF::QMareTextureWidget::mouseMoveEvent(QMouseEvent* e) {
	if (!(e->buttons() & Qt::LeftButton || e->buttons() & Qt::MiddleButton)) {
		return;
	}

	QPointF diff{e->position() - this->mousePressPosition};
	diff /= this->textureZoom;

	this->textureOffset.setX(std::clamp<double>(this->textureOffset.x() + diff.x(), static_cast<float>(this->width()) / -2.f, static_cast<float>(this->width()) / 2.f));
	this->textureOffset.setY(std::clamp<double>(this->textureOffset.y() + diff.y(), static_cast<float>(this->height()) / -2.f, static_cast<float>(this->height()) / 2.f));

	this->mousePressPosition = e->position();

	this->update();
	e->accept();
}

void MareTF::QMareTextureWidget::mousePressEvent(QMouseEvent* e) {
	if (!(e->buttons() & Qt::LeftButton || e->buttons() & Qt::MiddleButton)) {
		return;
	}

	this->mousePressPosition = e->position();
	this->setCursor({Qt::CursorShape::ClosedHandCursor});
}

void MareTF::QMareTextureWidget::mouseReleaseEvent(QMouseEvent*) {
	this->setCursor({Qt::CursorShape::ArrowCursor});
}

void MareTF::QMareTextureWidget::paintEvent(QPaintEvent*) {
	const auto availableWidth = this->width() * 0.85;
	const auto availableHeight = this->height() * 0.85;
	const auto aspectRatio = static_cast<double>(this->textureCurrent.width()) / this->textureCurrent.height();

	int actualWidth, actualHeight;
	if (aspectRatio > 1) {
		actualWidth = static_cast<int>(availableWidth);
		actualHeight = static_cast<int>(availableWidth / aspectRatio);
		if (actualHeight > availableHeight) {
			actualHeight = static_cast<int>(availableHeight);
			actualWidth = static_cast<int>(availableHeight * aspectRatio);
		}
	} else {
		actualHeight = static_cast<int>(availableHeight);
		actualWidth = static_cast<int>(availableHeight * aspectRatio);
		if (actualWidth > availableWidth) {
			actualWidth = static_cast<int>(availableWidth);
			actualHeight = static_cast<int>(availableWidth / aspectRatio);
		}
	}

	const QRect targetRect{
		static_cast<int>(this->textureOffset.x() * this->textureZoom + static_cast<float>(this->width()) / 2.f - static_cast<float>(actualWidth) * this->textureZoom / 2),
		static_cast<int>(this->textureOffset.y() * this->textureZoom + static_cast<float>(this->height()) / 2.f - static_cast<float>(actualHeight) * this->textureZoom / 2),
		static_cast<int>(static_cast<float>(actualWidth) * this->textureZoom),
		static_cast<int>(static_cast<float>(actualHeight) * this->textureZoom),
	};

	static constexpr QColor BACKGROUND_COLOR{20, 22, 24};

	QPainter painter{this};
	painter.fillRect(0, 0, this->width(), this->height(), BACKGROUND_COLOR);

	if (
		this->useA() && (
			(std::holds_alternative<vtfpp::PPL>(this->textureFile) && vtfpp::ImageFormatDetails::decompressedAlpha(std::get<vtfpp::PPL>(this->textureFile).getFormat())) ||
			(std::holds_alternative<vtfpp::VTF>(this->textureFile) && vtfpp::ImageFormatDetails::decompressedAlpha(std::get<vtfpp::VTF>(this->textureFile).getFormat()))
		)
	) {
		const QRect backgroundRect = this->rect().intersected(this->tiled ? targetRect.adjusted(-targetRect.width(), -targetRect.height(), targetRect.width(), targetRect.height()) : targetRect);
		if (this->useAMask()) {
			static constexpr auto SQUARE_SIZE = 32;
			for (int x = backgroundRect.left() / SQUARE_SIZE * SQUARE_SIZE; x < backgroundRect.right(); x += SQUARE_SIZE) {
				for (int y = backgroundRect.top() / SQUARE_SIZE * SQUARE_SIZE; y < backgroundRect.bottom(); y += SQUARE_SIZE) {
					if ((x / SQUARE_SIZE + y / SQUARE_SIZE) % 2 == 0) {
						painter.fillRect(qMax(x, backgroundRect.left()), qMax(y, backgroundRect.top()), qMin(SQUARE_SIZE, backgroundRect.right() - x + 1), qMin(SQUARE_SIZE, backgroundRect.bottom() - y + 1), {0xd6, 0xd6, 0xd6});
					} else {
						painter.fillRect(qMax(x, backgroundRect.left()), qMax(y, backgroundRect.top()), qMin(SQUARE_SIZE, backgroundRect.right() - x + 1), qMin(SQUARE_SIZE, backgroundRect.bottom() - y + 1), {0xf2, 0xf2, 0xf2});
					}
				}
			}
		} else {
			painter.fillRect(backgroundRect, Qt::black);
		}
	}

	const QRect srcRect{0, 0, this->textureCurrent.width(), this->textureCurrent.height()};
	painter.drawImage(targetRect, this->textureCurrent, srcRect);

	// Tiles
	if (this->tiled) {
		painter.drawImage(targetRect.adjusted(-targetRect.width(), -targetRect.height(), -targetRect.width(), -targetRect.height()), this->textureCurrent, srcRect);
		painter.drawImage(targetRect.adjusted(0,                   -targetRect.height(), 0,                   -targetRect.height()), this->textureCurrent, srcRect);
		painter.drawImage(targetRect.adjusted(targetRect.width(),  -targetRect.height(), targetRect.width(),  -targetRect.height()), this->textureCurrent, srcRect);
		painter.drawImage(targetRect.adjusted(targetRect.width(),  0,                    targetRect.width(),  0                   ), this->textureCurrent, srcRect);
		painter.drawImage(targetRect.adjusted(targetRect.width(),  targetRect.height(),  targetRect.width(),  targetRect.height() ), this->textureCurrent, srcRect);
		painter.drawImage(targetRect.adjusted(0,                   targetRect.height(),  0,                   targetRect.height() ), this->textureCurrent, srcRect);
		painter.drawImage(targetRect.adjusted(-targetRect.width(), targetRect.height(),  -targetRect.width(), targetRect.height() ), this->textureCurrent, srcRect);
		painter.drawImage(targetRect.adjusted(-targetRect.width(), 0,                    -targetRect.width(), 0                   ), this->textureCurrent, srcRect);
	}

	// Minimap
	if (!this->geometry().contains(targetRect)) {
		static constexpr auto MINIMAP_SCALE_FACTOR_MEDIUM = 5.5;
		static constexpr auto MINIMAP_MARGIN = 8;
		static constexpr auto MINIMAP_PADDING = 4;
		static constexpr auto MINIMAP_OVERLAY_BORDER_SIZE = 2;

		auto minimapBackgroundColor = BACKGROUND_COLOR.darker();
		minimapBackgroundColor.setAlpha(0xA0);
		auto minimapOverlayBorderColor = this->palette().color(QPalette::Highlight);
		minimapOverlayBorderColor.setAlpha(0xC0);
		auto minimapOverlayFillColor = this->palette().color(QPalette::Highlight);
		minimapOverlayFillColor.setAlpha(0x60);

		const auto referenceDim = static_cast<int>(qMin(this->rect().width(), this->rect().height()) / MINIMAP_SCALE_FACTOR_MEDIUM);

		QRect minimapTargetRect;
		if (aspectRatio > 1) {
			minimapTargetRect = QRect{this->rect().right() - referenceDim - MINIMAP_MARGIN, this->rect().top() + MINIMAP_MARGIN, referenceDim, static_cast<int>(static_cast<float>(referenceDim) / aspectRatio)};
		} else {
			const auto minimapWidth = static_cast<int>(static_cast<float>(referenceDim) * aspectRatio);
			minimapTargetRect = QRect{this->rect().right() - minimapWidth - MINIMAP_MARGIN, this->rect().top() + MINIMAP_MARGIN, minimapWidth, referenceDim};
		}
		const auto minimapViewportScaleX = static_cast<float>(minimapTargetRect.width()) / (static_cast<float>(actualWidth) * this->textureZoom);
		const auto minimapViewportScaleY = static_cast<float>(minimapTargetRect.height()) / (static_cast<float>(actualHeight) * this->textureZoom);

		painter.fillRect(minimapTargetRect.adjusted(-MINIMAP_PADDING, -MINIMAP_PADDING, MINIMAP_PADDING, MINIMAP_PADDING), minimapBackgroundColor);
		painter.drawImage(minimapTargetRect, this->textureCurrent.scaled(minimapTargetRect.width(), minimapTargetRect.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation), {0, 0, minimapTargetRect.width(), minimapTargetRect.height()});

		painter.setPen({minimapOverlayBorderColor, MINIMAP_OVERLAY_BORDER_SIZE});
		painter.setBrush({minimapOverlayFillColor});
		painter.drawRect(QRect{
			minimapTargetRect.left() + static_cast<int>(static_cast<float>(this->rect().left() - targetRect.left()) * minimapViewportScaleX),
			minimapTargetRect.top() + static_cast<int>(static_cast<float>(this->rect().top() - targetRect.top()) * minimapViewportScaleY),
			static_cast<int>(static_cast<float>(this->rect().width()) * minimapViewportScaleX),
			static_cast<int>(static_cast<float>(this->rect().height()) * minimapViewportScaleY)
		});
	}
}

void MareTF::QMareTextureWidget::resizeEvent(QResizeEvent* e) {
	if (!e->oldSize().width() || !e->oldSize().height()) {
		return;
	}
	this->textureOffset.setX(this->textureOffset.x() * e->size().width() / e->oldSize().width());
	this->textureOffset.setY(this->textureOffset.y() * e->size().height() / e->oldSize().height());
}

void MareTF::QMareTextureWidget::wheelEvent(QWheelEvent* e) {
	this->applyIncrementalZoom(static_cast<float>(e->angleDelta().y()) / 360.f);
	e->accept();
}

std::vector<std::byte> MareTF::QMareTextureWidget::Image::getImageDataAs(vtfpp::ImageFormat imageFormat, uint16_t frame) const {
	// todo: care about frame index
	return this->format == imageFormat ? this->imageData : vtfpp::ImageConversion::convertImageDataToFormat(this->imageData, this->format, imageFormat, this->width, this->height);
}

void TexturePreview::initPlugin(VWindowAccess_V6_0*) {}

const QSet<QString>& TexturePreview::getFileExtensions() const {
	static const QSet<QString> EXTENSIONS{
		// Images
		"apng",
		"bmp",
		"exr",
		"gif",
		"hdr",
		"jpeg",
		"jpg",
		"jxl",
		"pic",
		"png",
		"pgm",
		"ppm",
		"psd",
		"qoi",
		"tga",
		"webp",

		// PPL
		"ppl",

		// VTF
		"vtf",
		"xtf",
	};
	return EXTENSIONS;
}

int TexturePreview::getPriority() const {
	return 50;
}

QWidget* TexturePreview::getPreview(QWidget* parent) const {
	return new MareTF::QMareTextureWidget{parent};
}

// todo: there needs to be a panel with gizmos that say all the information and controls for changing the preview

QString TexturePreview::setData(QWidget* preview_, const QString& path, const quint8* dataPtr, quint64 dataLen) {
	auto* preview = qobject_cast<MareTF::QMareTextureWidget*>(preview_);
	if (!preview) {
		return UNEXPECTED_ERROR_MSG.toString();
	}

	if (const auto pathLower = path.toLower(); pathLower.endsWith("vtf") || pathLower.endsWith("xtf")) {
		// VTF loader
		if (!preview->loadVTF(dataPtr, dataLen)) {
			return tr("Failed to parse texture file contents.");
		}
		return {};
	} else if (pathLower.endsWith(".ppl")) {
		// PPL loader
		if (!preview->loadPPL(dataPtr, dataLen)) {
			return tr("Failed to parse texture file contents.");
		}
		return {};
	}

	// Image loader
	if (!preview->loadImage(dataPtr, dataLen)) {
		return tr("Failed to parse texture file contents.");
	}
	return {};
}
