#pragma once

#include <memory>
#include <vector>

#include <QImage>
#include <QWidget>
#include <VTFLib.h>

#include "../utility/VTFDecoder.h"

class QCheckBox;
class QSlider;
class QSpinBox;
class QLabel;

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

    [[nodiscard]] QString getVersion() const;
    [[nodiscard]] QString getFormat() const;
    [[nodiscard]] int getAuxCompression() const;
    [[nodiscard]] int getMaxFrame() const;
    [[nodiscard]] int getMaxFace() const;
    [[nodiscard]] int getMaxMip() const;
    [[nodiscard]] bool hasAlpha() const;
    [[nodiscard]] bool getAlphaEnabled() const;
    [[nodiscard]] bool getTileEnabled() const;
    [[nodiscard]] float getZoom() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    static inline constexpr struct
    {
        VTFImageFormat format;
        const char *name;
    } IMAGE_FORMATS[] = {
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
            //	{ IMAGE_FORMAT_NV_DST16, "NV_DST16" },
            //	{ IMAGE_FORMAT_NV_DST24, "NV_DST24" },
            //	{ IMAGE_FORMAT_NV_INTZ, "NV_INTZ" },
            //	{ IMAGE_FORMAT_NV_RAWZ, "NV_RAWZ" },
            //	{ IMAGE_FORMAT_ATI_DST16, "ATI_DST16" },
            //	{ IMAGE_FORMAT_ATI_DST24, "ATI_DST24" },
            { IMAGE_FORMAT_NV_NULL, "NV_NULL" },
            { IMAGE_FORMAT_ATI2N, "ATI2N" },
            { IMAGE_FORMAT_ATI1N, "ATI1N" },
    };

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
    QLabel *vtfVersionLabel;
    QLabel *vtfImageFormatLabel;
    QLabel *vtfAuxCompressionLabel;
};
