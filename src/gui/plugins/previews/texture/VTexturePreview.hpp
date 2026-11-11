#pragma once

#include <variant>

#include <vtfpp/vtfpp.h>

#include "../VPreviewPlugin.hpp"

class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QWheelEvent;

// Code in this block is modified from the MareTF GUI. Thanks, Laura!!! Love you 💙
namespace MareTF {

class QMareTextureWidget : public QWidget {
	Q_OBJECT;

public:
	explicit QMareTextureWidget(QWidget* parent = nullptr);

	[[nodiscard]] bool loadImage(const quint8* dataPtr, quint64 dataLen);

	[[nodiscard]] bool loadPPL(const quint8* dataPtr, quint64 dataLen);

	[[nodiscard]] bool loadPSFrames(const quint8* dataPtr, quint64 dataLen);

	[[nodiscard]] bool loadVTF(const quint8* dataPtr, quint64 dataLen);

	void reloadCurrentTexture();

	void applyIncrementalZoom(float increment);

	[[nodiscard]] uint8_t getCurrentMip() const;

	void setCurrentMip(uint8_t mip);

	[[nodiscard]] uint16_t getCurrentFrame() const;

	void setCurrentFrame(uint16_t frame);

	[[nodiscard]] uint8_t getCurrentFace() const;

	void setCurrentFace(uint8_t face);

	[[nodiscard]] uint16_t getCurrentDepth() const;

	void setCurrentDepth(uint16_t depth);

	[[nodiscard]] bool useR() const;

	[[nodiscard]] bool useG() const;

	[[nodiscard]] bool useB() const;

	void setRGB(bool newR, bool newG, bool newB);

	[[nodiscard]] bool useA() const;

	void setA(bool newA);

	[[nodiscard]] bool useAMask() const;

	void setAMask(bool newAMask);

	[[nodiscard]] bool useTiled() const;

	void setTiled(bool newTiled);

	[[nodiscard]] int getCurrentCubemapMode() const;

	void setCurrentCubemapMode(int mode);

	explicit operator bool() const;

protected:
	void mouseMoveEvent(QMouseEvent* e) override;

	void mousePressEvent(QMouseEvent* e) override;

	void mouseReleaseEvent(QMouseEvent* e) override;

	void paintEvent(QPaintEvent*) override;

	void resizeEvent(QResizeEvent* e) override;

	void wheelEvent(QWheelEvent* e) override;

	struct Image {
		std::vector<std::byte> imageData;
		vtfpp::ImageFormat format = vtfpp::ImageFormat::EMPTY;
		int width = 0, height = 0, frameCount = 0;

		[[nodiscard]] std::vector<std::byte> getImageDataAs(vtfpp::ImageFormat imageFormat, uint16_t frame = 0) const;
	};

	std::variant<std::monostate, Image, vtfpp::PPL, vtfpp::PSFrames, vtfpp::VTF> textureFile;
	std::vector<std::byte> textureCurrentData;
	QImage textureCurrent;
	QPointF textureOffset;
	float textureZoom = 1.f;
	uint8_t currentMip = 0;
	uint8_t currentFace = 0;
	uint16_t currentFrame = 0;
	uint16_t currentDepth = 0;
	bool r = true;
	bool g = true;
	bool b = true;
	bool a = true;
	bool aMask = true;
	bool tiled = false;
	int cubemapMode = 0;

	QPointF mousePressPosition;
};

} // namespace MareTF

class VTexturePreview final : public VPreviewPlugin_V6_0 {
	Q_OBJECT;
	Q_PLUGIN_METADATA(IID VPreviewPlugin_V6_0_iid FILE "VTexturePreview.json");
	Q_INTERFACES(VPreviewPlugin_V6_0);

public:
	void initPlugin(VWindowAccess_V6_0*) override;

	[[nodiscard]] const QSet<QString>& getFileExtensions() const override;

	[[nodiscard]] int getPriority() const override;

	[[nodiscard]] QWidget* getPreview(QWidget* parent) const override;

	[[nodiscard]] QString setData(QWidget* preview_, const QString&, const quint8* dataPtr, quint64 dataLen) override;
};
