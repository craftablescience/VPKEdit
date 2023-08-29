#include <memory>
#include <vector>

#include <QImage>
#include <QWidget>
#include <VTFLib.h>

class QSlider;

class Image : public QWidget {
    Q_OBJECT;

public:
    explicit Image(QWidget* parent = nullptr);

    void setImage(const std::vector<std::byte>& data);

    void setZoom(int zoom_);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage image;
    float zoom;
};

class ImagePreview : public QWidget {
    Q_OBJECT;

public:
    explicit ImagePreview(QWidget* parent = nullptr);

    void setImage(const std::vector<std::byte>& data);

private:
    Image* image;
    QSlider* zoomSlider;
};
