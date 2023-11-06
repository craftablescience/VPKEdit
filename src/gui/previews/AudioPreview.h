#pragma once

#include <QWidget>

class AudioPreview : public QWidget {
    Q_OBJECT;

public:
    static inline const QStringList EXTENSIONS {
        ".mp3",
        ".wav",
        ".ogg",
    };

    explicit AudioPreview(QWidget* parent = nullptr);

    void setAudio(const std::vector<std::byte>& data, const QString& filename);
};
