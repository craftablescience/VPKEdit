#pragma once

#include <QWidget>

class ErrorPreview : public QWidget {
    Q_OBJECT;

public:
    static inline const QStringList EXTENSIONS {
        // None, this is displayed when an entry cannot be read, maybe due to another process using it
    };

    explicit ErrorPreview(QWidget* parent = nullptr);
};
