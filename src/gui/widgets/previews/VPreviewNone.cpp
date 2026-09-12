#include "VPreviewNone.hpp"

#include <QLabel>

#include "Config.hpp"

VPreviewNone::VPreviewNone(QWidget* parent) : VPreviewInfo({":/logo.png"}, PROJECT_TITLE, parent) {
	// We want the logo and text to be consistently greyed out
	this->icon->setDisabled(true);
	this->text->setDisabled(true);
}
