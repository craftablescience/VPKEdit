#pragma once

#include <memory>
#include <vector>

#include <QImage>
#include <QWidget>
#include <VTFLib.h>

#include "../utility/VTFDecoder.h"

class QCheckBox;
class QLabel;
class QSlider;
class QSpinBox;

class VTFWidget : public QWidget {
    Q_OBJECT;

public:
    explicit VTFWidget(QWidget* parent = nullptr);

    void setData(const std::vector<std::byte>& data);

    void setFrame(int frame);
    void setFace(int face);
    void setMip(int mip);
    void setAlphaEnabled(bool alpha);
    void setTileEnabled(bool tile);
    void setZoom(int zoom_);

    [[nodiscard]] int getMaxFrame() const;

    [[nodiscard]] int getMaxFace() const;

    [[nodiscard]] int getMaxMip() const;

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
    std::unique_ptr<VTFLib::CVTFFile> vtf;

    QImage image;
    VTFData vtfData;

    int currentFace;
    int currentFrame;
    int currentMip;
    bool alphaEnabled;
    bool tileEnabled;
    float zoom;

    void decodeImage(int face, int frame, int mip, bool alpha);
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
    QSpinBox* frameSpin;
    QSpinBox* faceSpin;
    QSpinBox* mipSpin;
    QCheckBox* alphaCheckBox;
    QCheckBox* tileCheckBox;
    QSlider* zoomSlider;
    QLabel* versionLabel;
    QLabel* imageFormatLabel;
    QLabel* compressionLevelLabel;
};
