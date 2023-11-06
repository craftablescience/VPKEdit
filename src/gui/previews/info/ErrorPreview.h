#pragma once

#include "AbstractInfoPreview.h"

class ErrorPreview : public AbstractInfoPreview {
    Q_OBJECT;

public:
    static inline const QStringList EXTENSIONS {
        // None, this is displayed when an entry cannot be read, maybe due to another process using it
    };

    explicit ErrorPreview(QWidget* parent = nullptr);
};
