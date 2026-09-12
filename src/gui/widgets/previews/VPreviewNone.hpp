#pragma once

#include "VPreviewInfo.hpp"

class VPreviewNone : public VPreviewInfo {
	Q_OBJECT;

public:
	explicit VPreviewNone(QWidget* parent = nullptr);
};
