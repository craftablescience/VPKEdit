#pragma once

#include <cstddef>
#include <optional>
#include <tuple>

#include <QImage>
#include <VTFLib.h>

struct VTFData {
	std::unique_ptr<std::byte[]> data;
	unsigned int dataSize;
	unsigned int width;
	unsigned int height;
	unsigned int depth;
	QImage::Format format;
};

namespace VTFDecoder {

std::optional<VTFData> decodeImage(const VTFLib::CVTFFile& vtf, bool alpha);

std::optional<VTFData> decodeImage(const VTFLib::CVTFFile& vtf, int face = 0, int frame = 0, int mip = 0, bool alpha = false);

} // namespace VTFDecoder
