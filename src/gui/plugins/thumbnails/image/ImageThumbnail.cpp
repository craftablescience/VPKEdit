#include "ImageThumbnail.hpp"

#include <vtfpp/ImageConversion.h>

void ImageThumbnail::initPlugin(VWindowAccess_V6_0*) {}

const QSet<QString>& ImageThumbnail::getFileExtensions() const {
	static const QSet<QString> EXTENSIONS{
		"apng",
		"bmp",
		"exr",
		"gif",
		"hdr",
		"jpeg",
		"jpg",
		"jxl",
		"pic",
		"png",
		"pgm",
		"ppm",
		"psd",
		"qoi",
		"tga",
		"webp",
	};
	return EXTENSIONS;
}

int ImageThumbnail::getPriority() const {
	return 50;
}

QPixmap ImageThumbnail::getThumbnail(const QString&, const quint8* dataPtr, quint64 dataLen) {
	vtfpp::ImageFormat imageFormat;
	int width, height, frameCount;
	auto imageData = vtfpp::ImageConversion::convertFileToImageData({reinterpret_cast<const std::byte*>(dataPtr), dataLen}, imageFormat, width, height, frameCount);
	if (imageData.empty() || width <= 0 || height <= 0 || frameCount <= 0) {
		return {};
	}
	if (frameCount > 1) {
		imageData.resize(imageData.size() / frameCount);
	}
	if (imageFormat == vtfpp::ImageFormat::RGBA8888) {
		return QPixmap::fromImage(QImage{reinterpret_cast<uchar*>(imageData.data()), width, height, QImage::Format_RGBA8888});
	}
	imageData = vtfpp::ImageConversion::convertImageDataToFormat(imageData, imageFormat, vtfpp::ImageFormat::RGBA8888, width, height);
	return QPixmap::fromImage(QImage{reinterpret_cast<uchar*>(imageData.data()), width, height, QImage::Format_RGBA8888});
}
