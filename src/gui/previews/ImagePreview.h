#pragma once

#include <memory>
#include <vector>

#include <QImage>
#include <QWidget>
#include <VTFLib.h>

class QCheckBox;
class QSlider;

class Image : public QWidget {
    Q_OBJECT;

public:
    explicit Image(QWidget* parent = nullptr);

    void setImage(const std::vector<std::byte>& data);

    void setAlphaEnabled(bool alpha);
    void setTileEnabled(bool tile);
    void setZoom(int zoom_);

    [[nodiscard]] bool hasAlpha() const;
    [[nodiscard]] bool getAlphaEnabled() const;
    [[nodiscard]] bool getTileEnabled() const;
    [[nodiscard]] float getZoom() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage image;
    bool alphaEnabled;
    bool tileEnabled;
    float zoom;
};

class ImagePreview : public QWidget {
    Q_OBJECT;

public:
    static inline const QStringList EXTENSIONS {
        ".tga",
        ".jpg",
        ".jpeg",
        ".jfif",
        ".png",
        ".webp",
        ".bmp",
    };

    explicit ImagePreview(QWidget* parent = nullptr);

    void setImage(const std::vector<std::byte>& data) const;

protected:
    void wheelEvent(QWheelEvent* event) override;

private:
    Image* image;
    QCheckBox* alphaCheckBox;
    QCheckBox* tileCheckBox;
    QSlider* zoomSlider;
};
