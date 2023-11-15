#pragma once

#include "AbstractInfoPreview.h"

class InvalidMDLErrorPreview : public AbstractInfoPreview {
    Q_OBJECT;

public:
    static inline const QStringList EXTENSIONS{};

    explicit InvalidMDLErrorPreview(QWidget* parent = nullptr);

	void setErrorMessage(const QString& error);
};
