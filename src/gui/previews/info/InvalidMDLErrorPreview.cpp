#include "InvalidMDLErrorPreview.h"

InvalidMDLErrorPreview::InvalidMDLErrorPreview(QWidget* parent)
        : AbstractInfoPreview({":/error.png"}, "", parent) {}

void InvalidMDLErrorPreview::setErrorMessage(const QString& error) {
	this->error->setText(error);
}
