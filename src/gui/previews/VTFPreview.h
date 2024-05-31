#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <QImage>
#include <QWidget>
#include <vtfpp/vtfpp.h>

class QCheckBox;
class QLabel;
class QSlider;
class QSpinBox;

class VTFWidget : public QWidget {
	Q_OBJECT;

public:
	explicit VTFWidget(QWidget* parent = nullptr);

	void setData(const std::vector<std::byte>& data);

	void setShowEverythingEnabled(bool show);

	void setMip(int mip);

	void setFrame(int frame);

	void setFace(int face);

	void setSlice(int slice);

	void setAlphaEnabled(bool alpha);

	void setTileEnabled(bool tile);

	void setZoom(int zoom_);

	[[nodiscard]] bool getShowEverythingEnabled() const;

	[[nodiscard]] int getMaxMip() const;

	[[nodiscard]] int getMaxFrame() const;

	[[nodiscard]] int getMaxFace() const;

	[[nodiscard]] int getMaxSlice() const;

	[[nodiscard]] bool hasAlpha() const;

	[[nodiscard]] bool getAlphaEnabled() const;

	[[nodiscard]] bool getTileEnabled() const;

	[[nodiscard]] float getZoom() const;

	[[nodiscard]] QString getVersion() const;

	[[nodiscard]] QString getFormat() const;

	[[nodiscard]] int getAuxCompression() const;

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	std::unique_ptr<vtfpp::VTF> vtf;
	std::vector<std::byte> vtfData;
	QImage image;

	bool showEverything;
	int currentMip;
	int currentFrame;
	int currentFace;
	int currentSlice;
	bool alphaEnabled;
	bool tileEnabled;
	float zoom;

	void decodeImage(int mip, int frame, int face, int slice, bool alpha);
};

class VTFPreview : public QWidget {
	Q_OBJECT;

public:
	static inline const QStringList EXTENSIONS {
		".vtf",
	};

	explicit VTFPreview(QWidget* parent = nullptr);

	void setData(const std::vector<std::byte>& data) const;

protected:
	void wheelEvent(QWheelEvent* event) override;

private:
	VTFWidget* vtf;
	QCheckBox* showEverythingCheckBox;
	QSpinBox* mipSpin;
	QSpinBox* frameSpin;
	QSpinBox* faceSpin;
	QSpinBox* sliceSpin;
	QCheckBox* alphaCheckBox;
	QCheckBox* tileCheckBox;
	QSlider* zoomSlider;
	QLabel* versionLabel;
	QLabel* imageFormatLabel;
	QLabel* compressionLevelLabel;
};
