#include "NoAvailablePreview.h"

NoAvailablePreview::NoAvailablePreview(QWidget* parent)
		: AbstractInfoPreview({":/warning.png"}, tr("No available preview."), parent) {
	// We want the logo to be consistently greyed out
	this->setDisabled(true);
}
