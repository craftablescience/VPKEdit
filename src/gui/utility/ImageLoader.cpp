#include "ImageLoader.h"

#include <cstring>

#include <sourcepp/FS.h>
#include <vtfpp/ImageConversion.h>

using namespace sourcepp;
using namespace vtfpp;

QImage ImageLoader::load(const QString& imagePath) {
	static const QImage INVALID_ICON{":/icons/missing.png"};

	if (imagePath.isEmpty()) {
		return INVALID_ICON;
	}

	{
		QImage image;
		if (image.load(imagePath)) {
			return image;
		}
	}

	const auto srcBuffer = fs::readFileBuffer(imagePath.toLocal8Bit().constData());
	if (srcBuffer.empty()) {
		return INVALID_ICON;
	}

	ImageFormat format;
	int width, height, frameCount;
	auto imageBuffer = ImageConversion::convertFileToImageData(srcBuffer, format, width, height, frameCount);
	if (format == ImageFormat::EMPTY || width <= 0 || height <= 0 || frameCount <= 0) {
		return INVALID_ICON;
	}

	if (frameCount > 1) {
		imageBuffer.resize(width * height * (ImageFormatDetails::bpp(format) / 8));
	}

	const auto imageBufferRGBA8888 = ImageConversion::convertImageDataToFormat(imageBuffer, format, ImageFormat::RGBA8888, static_cast<uint16_t>(width), static_cast<uint16_t>(height));

	// Forgive me daddy for I have been naughty
	auto* imageBufferRGBA8888Copy = new unsigned char[imageBufferRGBA8888.size()];
	std::memcpy(imageBufferRGBA8888Copy, imageBufferRGBA8888.data(), imageBufferRGBA8888.size());
	return {imageBufferRGBA8888Copy, width, height, QImage::Format_RGBA8888, [](void* buf) {
		delete[] reinterpret_cast<unsigned char*>(buf);
	}, imageBufferRGBA8888Copy};
}

QImage ImageLoader::load(const std::vector<std::byte>& imageData) {
	static const QImage INVALID_ICON{":/icons/missing.png"};

	{
		QImage image;
		if (image.loadFromData(imageData)) {
			return image;
		}
	}

	if (imageData.empty()) {
		return INVALID_ICON;
	}

	ImageFormat format;
	int width, height, frameCount;
	auto imageBuffer = ImageConversion::convertFileToImageData(imageData, format, width, height, frameCount);
	if (format == ImageFormat::EMPTY || width <= 0 || height <= 0 || frameCount <= 0) {
		return INVALID_ICON;
	}

	if (frameCount > 1) {
		imageBuffer.resize(width * height * (ImageFormatDetails::bpp(format) / 8));
	}

	const auto imageBufferRGBA8888 = ImageConversion::convertImageDataToFormat(imageBuffer, format, ImageFormat::RGBA8888, static_cast<uint16_t>(width), static_cast<uint16_t>(height));

	// Forgive me daddy for I have been naughty
	auto* imageBufferRGBA8888Copy = new unsigned char[imageBufferRGBA8888.size()];
	std::memcpy(imageBufferRGBA8888Copy, imageBufferRGBA8888.data(), imageBufferRGBA8888.size());
	return {imageBufferRGBA8888Copy, width, height, QImage::Format_RGBA8888, [](void* buf) {
		delete[] reinterpret_cast<unsigned char*>(buf);
	}, imageBufferRGBA8888Copy};
}
