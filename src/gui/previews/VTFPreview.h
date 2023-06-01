#pragma once

#include <memory>
#include <vector>

#include <QImage>
#include <QWidget>
#include <VTFLib.h>

class QSlider;
class QSpinBox;

class VTFImage : public QWidget {
    Q_OBJECT;

public:
    explicit VTFImage(QWidget* parent = nullptr);

    void setImage(const std::vector<std::byte>& data);

    void setFrame(int frame);
    void setFace(int face);
    void setMip(int mip);
    void setZoom(int zoom_);

    int getMaxFrame();
    int getMaxFace();
    int getMaxMip();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::unique_ptr<VTFLib::CVTFFile> vtf;

    QImage image;
    std::unique_ptr<std::byte[]> imageData;

    int currentFace;
    int currentFrame;
    int currentMip;
    float zoom;

    void decodeImage(int face, int frame, int mip);
};

class VTFPreview : public QWidget {
    Q_OBJECT;

public:
    explicit VTFPreview(QWidget* parent = nullptr);

    void setImage(const std::vector<std::byte>& data);

private:
    VTFImage* image;
    QSpinBox* frameSpin;
    QSpinBox* faceSpin;
    QSpinBox* mipSpin;
    QSlider* zoomSlider;
};
