#pragma once

#include "AbstractInfoPreview.h"

class FileLoadErrorPreview : public AbstractInfoPreview {
    Q_OBJECT;

public:
    static inline const QStringList EXTENSIONS{};

    explicit FileLoadErrorPreview(QWidget* parent = nullptr);
};
