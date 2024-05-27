#include "TGADecoder.h"

#include <cstdint>
#include <fstream>

#pragma pack(push, 1)

struct TGAHeader {
	std::uint8_t idLength;
	[[maybe_unused]] std::uint8_t colorMapType;
	std::uint8_t dataTypeCode;
	[[maybe_unused]] std::int16_t colorMapOrigin;
	[[maybe_unused]] std::int16_t colorMapLength;
	std::uint8_t colorMapDepth;
	[[maybe_unused]] std::int16_t xOrigin;
	[[maybe_unused]] std::int16_t yOrigin;
	std::int16_t width;
	std::int16_t height;
	std::uint8_t bpp;
	[[maybe_unused]] std::uint8_t imageDescriptor;
};

#pragma pack(pop)

// Modified from https://forum.qt.io/topic/74712/qimage-from-tga-with-alpha/11
// Thank you Qt for not supporting this natively
std::optional<QImage> TGADecoder::decodeImage(const QString& imagePath) {
	// Try loading through Qt
	QImage image;
	if (image.load(imagePath)) {
		return image;
	}

	// Create stream
	std::ifstream imageStream{imagePath.toStdString(), std::ios::binary};
	if (!imageStream) {
		return std::nullopt;
	}
	imageStream >> std::noskipws;

	// Parse header
	TGAHeader header{};
	imageStream.read(reinterpret_cast<char*>(&header), sizeof(TGAHeader));

	// Bail if we can't parse it
	if (header.bpp != 24 && header.bpp != 32) {
		return std::nullopt;
	}

	// Get ready to parse image data
	std::uint32_t dataSize = header.width * header.height * header.bpp / 8;
	std::vector<std::uint8_t> imageData;
	imageData.resize(dataSize);
	imageStream.seekg(header.idLength + header.colorMapDepth, std::ios_base::cur);

	// Parse image data
	bool shouldFlip = false;
	if (header.dataTypeCode == 2) {
		// Uncompressed RGB
		imageStream.read(reinterpret_cast<char*>(imageData.data()), dataSize);
		shouldFlip = true;
	} else if (header.dataTypeCode == 10) {
		// RLE-Compressed RGB
		std::uint8_t tempChunkHeader;
		std::uint8_t tempData[5];
		std::uint32_t tempByteIndex = 0;

		do {
			imageStream.read(reinterpret_cast<char*>(&tempChunkHeader), sizeof(tempChunkHeader));

			if (tempChunkHeader >> 7) {
				// repeat count
				// just use the first 7 bits
				tempChunkHeader = (std::uint8_t(tempChunkHeader << 1) >> 1);

				imageStream.read(reinterpret_cast<char*>(&tempData), header.bpp / 8);

				for (int i = 0; i <= tempChunkHeader; i++) {
					imageData.at(tempByteIndex++) = tempData[0];
					imageData.at(tempByteIndex++) = tempData[1];
					imageData.at(tempByteIndex++) = tempData[2];
					if (header.bpp == 32) imageData.at(tempByteIndex++) = tempData[3];
				}
			} else {
				// data count
				// just use the first 7 bits
				tempChunkHeader = (std::uint8_t(tempChunkHeader << 1) >> 1);

				for (int i = 0; i <= tempChunkHeader; i++) {
					imageStream.read(reinterpret_cast<char*>(&tempData), header.bpp / 8);

					imageData.at(tempByteIndex++) = tempData[0];
					imageData.at(tempByteIndex++) = tempData[1];
					imageData.at(tempByteIndex++) = tempData[2];
					if (header.bpp == 32) imageData.at(tempByteIndex++) = tempData[3];
				}
			}
		} while (tempByteIndex < dataSize);
	} else {
		return std::nullopt;
	}

	image = QImage(header.width, header.height, QImage::Format_RGBA8888);
	int pixelSize = header.bpp == 32 ? 4 : 3;
	for (int x = 0; x < header.width; x++) {
		for (int y = 0; y < header.height; y++) {
			int valB = imageData.at(y * header.width * pixelSize + x * pixelSize);
			int valG = imageData.at(y * header.width * pixelSize + x * pixelSize + 1);
			int valR = imageData.at(y * header.width * pixelSize + x * pixelSize + 2);
			int valA = pixelSize == 4 ? imageData.at(y * header.width * pixelSize + x * pixelSize + 3) : 255;

			QColor value{valR, valG, valB, valA};
			image.setPixelColor(x, y, value);
		}
	}

	if (shouldFlip) {
		image = image.mirrored();
	}
	return image;
}
