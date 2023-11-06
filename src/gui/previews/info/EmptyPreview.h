#pragma once

#include "AbstractInfoPreview.h"

class EmptyPreview : public AbstractInfoPreview {
    Q_OBJECT;

public:
    static inline const QStringList EXTENSIONS {
        // None, this is displayed when there is nothing being previewed
    };

    explicit EmptyPreview(QWidget* parent = nullptr);
};
