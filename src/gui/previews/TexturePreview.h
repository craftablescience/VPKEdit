#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <QImage>
#include <QMetaEnum>
#include <QWidget>
#include <vtfpp/vtfpp.h>

class QCheckBox;
class QLabel;
class QSlider;
class QSpinBox;

class FileViewer;

class ITextureWidget : public QWidget {
	Q_OBJECT;

public:
	static constexpr float DEFAULT_ZOOM = 100;
	static constexpr float MAX_ZOOM = 1500;
	static constexpr float MIN_ZOOM = 10;

	explicit ITextureWidget(QWidget* parent = nullptr);

	virtual void setData(const std::vector<std::byte>& data) = 0;

	void setShowEverythingEnabled(bool show) { this->showEverything = show; }

	virtual void setMip(int mip) = 0;

	virtual void setFrame(int frame) = 0;

	virtual void setFace(int face) = 0;

	virtual void setSlice(int slice) = 0;

	virtual void setAlphaEnabled(bool alpha) = 0;

	void setTileEnabled(bool tile) { this->tileEnabled = tile; }

	void setZoom(int zoom_, bool emitSignal = false);

	void setOffset(const QPoint& off) { this->offset = off; }

	[[nodiscard]] QPoint getOffset() const { return this->offset; }

	[[nodiscard]] virtual uint16_t getCurrentImageWidth() const = 0;

	[[nodiscard]] virtual uint16_t getCurrentImageHeight() const = 0;

	[[nodiscard]] virtual uint16_t getCurrentImageDepth() const = 0;

	[[nodiscard]] bool getShowEverythingEnabled() const { return this->showEverything; }

	[[nodiscard]] virtual int getMaxMip() const = 0;

	[[nodiscard]] virtual int getMaxFrame() const = 0;

	[[nodiscard]] virtual int getMaxFace() const = 0;

	[[nodiscard]] virtual bool hasAlpha() const = 0;

	[[nodiscard]] bool getAlphaEnabled() const { return this->alphaEnabled; }

	[[nodiscard]] bool getTileEnabled() const { return this->tileEnabled; }

	[[nodiscard]] float getZoom() const { return this->zoom * 100.f; }

	[[nodiscard]] virtual QString getVersion() const = 0;

	[[nodiscard]] virtual QString getFormat() const = 0;

	[[nodiscard]] virtual int getAuxCompression() const = 0;

signals:
	void zoomUpdated(float newZoom);

protected:
	[[nodiscard]] float getScaledZoom() const;

	std::vector<std::byte> imageData;
	QImage image;

	bool showEverything = false;
	int currentMip = 0;
	int currentFrame = 0;
	int currentFace = 0;
	int currentSlice = 0;
	bool alphaEnabled = false;
	bool tileEnabled = false;
	float zoom = 1.f;
	QPoint offset;
};

class ImageWidget : public ITextureWidget {
	Q_OBJECT;

public:
	using ITextureWidget::ITextureWidget;

	void setData(const std::vector<std::byte>& data) override;

	void setMip(int) override {}

	void setFrame(int) override {}

	void setFace(int) override {}

	void setSlice(int) override {}

	void setAlphaEnabled(bool alpha) override { this->alphaEnabled = alpha; }

	[[nodiscard]] uint16_t getCurrentImageWidth() const override { return this->image.width(); }

	[[nodiscard]] uint16_t getCurrentImageHeight() const override { return this->image.height(); }

	[[nodiscard]] uint16_t getCurrentImageDepth() const override { return 1; }

	[[nodiscard]] int getMaxMip() const override { return 1; }

	[[nodiscard]] int getMaxFrame() const override { return 1; }

	[[nodiscard]] int getMaxFace() const override { return 1; }

	[[nodiscard]] bool hasAlpha() const override { return this->image.hasAlphaChannel(); }

	[[nodiscard]] QString getVersion() const override { return "N/A"; }

	[[nodiscard]] QString getFormat() const override;

	[[nodiscard]] int getAuxCompression() const override { return 0; }

protected:
	void paintEvent(QPaintEvent* event) override;
};

class SVGWidget : public ImageWidget {
	Q_OBJECT;

public:
	using ImageWidget::ImageWidget;

	void setData(const std::vector<std::byte>& data) override;

	[[nodiscard]] QString getFormat() const override { return "SVG"; }
};

class PPLWidget : public ITextureWidget {
	Q_OBJECT;

public:
	using ITextureWidget::ITextureWidget;

	void setData(const std::vector<std::byte>& data) override;

	void setMip(int) override {}

	void setFrame(int) override {}

	void setFace(int) override {}

	void setSlice(int) override {}

	void setAlphaEnabled(bool alpha) override {}

	[[nodiscard]] uint16_t getCurrentImageWidth() const override { return this->ppl ? this->ppl->getImageRaw(this->currentFrame)->width : 0; }

	[[nodiscard]] uint16_t getCurrentImageHeight() const override { return this->ppl ? this->ppl->getImageRaw(this->currentFrame)->height : 0; }

	[[nodiscard]] uint16_t getCurrentImageDepth() const override { return 1; }

	[[nodiscard]] int getMaxMip() const override { return 1; }

	[[nodiscard]] int getMaxFrame() const override { return 1; }

	[[nodiscard]] int getMaxFace() const override { return 1; }

	[[nodiscard]] bool hasAlpha() const override;

	[[nodiscard]] QString getVersion() const override;

	[[nodiscard]] QString getFormat() const override;

	[[nodiscard]] int getAuxCompression() const override { return 0; }

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	std::unique_ptr<vtfpp::PPL> ppl;

	void decodeImage();
};

class VTFWidget : public ITextureWidget {
	Q_OBJECT;

public:
	using ITextureWidget::ITextureWidget;

	void setData(const std::vector<std::byte>& data) override;

	void setMip(int mip) override { this->decodeImage(mip, this->currentFrame, this->currentFace, this->currentSlice, this->alphaEnabled); }

	void setFrame(int frame) override { this->decodeImage(this->currentMip, frame, this->currentFace, this->currentSlice, this->alphaEnabled); }

	void setFace(int face) override { this->decodeImage(this->currentMip, this->currentFrame, face, this->currentSlice, this->alphaEnabled); }

	void setSlice(int slice) override { this->decodeImage(this->currentMip, this->currentFrame, this->currentFace, slice, this->alphaEnabled); }

	void setAlphaEnabled(bool alpha) override { this->decodeImage(this->currentMip, this->currentFrame, this->currentFace, this->currentSlice, alpha); }

	[[nodiscard]] uint16_t getCurrentImageWidth() const override { return this->vtf->getWidth(this->currentMip); }

	[[nodiscard]] uint16_t getCurrentImageHeight() const override { return this->vtf->getHeight(this->currentMip); }

	[[nodiscard]] uint16_t getCurrentImageDepth() const override { return this->vtf->getDepth(); }

	[[nodiscard]] int getMaxMip() const override { return this->vtf->getMipCount(); }

	[[nodiscard]] int getMaxFrame() const override { return this->vtf->getFrameCount(); }

	[[nodiscard]] int getMaxFace() const override { return this->vtf->getFaceCount(); }

	[[nodiscard]] bool hasAlpha() const override;

	[[nodiscard]] QString getVersion() const override;

	[[nodiscard]] QString getFormat() const override;

	[[nodiscard]] int getAuxCompression() const override;

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	std::unique_ptr<vtfpp::VTF> vtf;

	void decodeImage(int mip, int frame, int face, int slice, bool alpha);
};

class TexturePreview : public QWidget {
	Q_OBJECT;

public:
	static inline const QStringList EXTENSIONS_IMAGE {
		".tga",
		".jpg",
		".jpeg",
		".jfif",
		".png",
		".apng",
		".webp",
		".bmp",
		".qoi",
		".psd",
		".gif",
		".hdr",
		".exr",
		".pic",
		".ppm",
		".pgm",
	};

	static inline const QStringList EXTENSIONS_SVG {
		".svg",
	};

	static inline const QStringList EXTENSIONS_PPL {
		".ppl",
	};

	static inline const QStringList EXTENSIONS_TTX {
		".tth",
		".ttz",
	};

	static inline const QStringList EXTENSIONS_VTF {
		".vtf",
		".xtf",
	};

	explicit TexturePreview(QWidget* parent = nullptr, FileViewer* fileViewer = nullptr);

	void setImageData(const std::vector<std::byte>& data) const;

	void setSVGData(const std::vector<std::byte>& data) const;

	void setPPLData(const std::vector<std::byte>& data) const;

	void setTTXData(const std::vector<std::byte>& tthData, const std::vector<std::byte>& ttzData) const;

	void setVTFData(const std::vector<std::byte>& data) const;

protected:
	void setData(ITextureWidget* widget) const;

	[[nodiscard]] ITextureWidget* getVisibleWidget() const;

	void mouseMoveEvent(QMouseEvent* event) override;

	void mousePressEvent(QMouseEvent* event) override;

	void mouseReleaseEvent(QMouseEvent* event) override;

	void wheelEvent(QWheelEvent* event) override;

private:
	FileViewer* fileViewer;

	ITextureWidget* image;
	ITextureWidget* svg;
	ITextureWidget* ppl;
	ITextureWidget* vtf;
	QCheckBox* showEverythingCheckBox;
	QSpinBox* mipSpin;
	QSpinBox* frameSpin;
	QSpinBox* faceSpin;
	QSpinBox* sliceSpin;
	QCheckBox* alphaCheckBox;
	QCheckBox* tileCheckBox;
	QSlider* zoomSlider;
	QLabel* sizeLabel;
	QLabel* versionLabel;
	QLabel* imageFormatLabel;
	QLabel* compressionLevelLabel;
	bool dragging = false;
	QPoint lastDrag;
};
