#pragma once

#include <cstddef>
#include <optional>
#include <tuple>

#include <QImage>
#include <QVector>
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

std::optional<VTFData> decodeImage(const VTFLib::CVTFFile& vtf, int face, int frame, int mip, bool alpha);

} // namespace VTFDecoder
