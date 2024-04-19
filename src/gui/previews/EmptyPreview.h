#pragma once

#include "InfoPreview.h"

class EmptyPreview : public InfoPreview {
	Q_OBJECT;

public:
	static inline const QStringList EXTENSIONS{};

	explicit EmptyPreview(QWidget* parent = nullptr);
};
