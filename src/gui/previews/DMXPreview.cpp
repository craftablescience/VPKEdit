#include "DMXPreview.h"

#include <dmxpp/dmxpp.h>

#include "../FileViewer.h"

using namespace dmxpp;

DMXPreview::DMXPreview(FileViewer* fileViewer_, QWidget* parent)
		: QWidget(parent)
		, fileViewer(fileViewer_) {}

void DMXPreview::setData(const std::vector<std::byte>& data) const {
	DMX dmx{data};
	if (!dmx) {
		this->fileViewer->showInfoPreview({":/icons/error.png"}, tr("Failed to parse DMX file."));
		return;
	}
}
