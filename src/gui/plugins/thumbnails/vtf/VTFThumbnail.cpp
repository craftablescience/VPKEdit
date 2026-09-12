#include "VTFThumbnail.hpp"

#include <vtfpp/VTF.h>

void VTFThumbnail::initPlugin(IVPKEditWindowAccess_V6_0*) {}

const QSet<QString>& VTFThumbnail::getFileExtensions() const {
	static const QSet<QString> EXTENSIONS{
		"vtf",
		"xtf",
	};
	return EXTENSIONS;
}

int VTFThumbnail::getPriority() const {
	return 50;
}

QPixmap VTFThumbnail::getThumbnail(const QString&, const quint8* dataPtr, quint64 dataLen) {
	const vtfpp::VTF vtf{{reinterpret_cast<const std::byte*>(dataPtr), dataLen}};
	if (!vtf) {
		return {};
	}

	auto imageData = vtf.getImageDataAsRGBA8888();
	return QPixmap::fromImage(QImage{reinterpret_cast<uchar*>(imageData.data()), vtf.getWidth(), vtf.getHeight(), QImage::Format_RGBA8888});
}
