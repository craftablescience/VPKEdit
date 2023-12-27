#pragma once

#include "AbstractInfoPreview.h"

class NoAvailablePreview : public AbstractInfoPreview {
	Q_OBJECT;

public:
	static inline const QStringList EXTENSIONS{};

	explicit NoAvailablePreview(QWidget* parent = nullptr);
};
