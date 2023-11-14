#include "VTFDecoder.h"

using namespace VTFLib;

std::optional<VTFData> VTFDecoder::decodeImage(const CVTFFile& vtf, int face, int frame, int mip, bool alpha) {
	// Compute draw size for this mip, frame, etc
	vlUInt imageWidth, imageHeight, imageDepth;
	CVTFFile::ComputeMipmapDimensions(vtf.GetWidth(), vtf.GetHeight(), vtf.GetDepth(), mip, imageWidth, imageHeight, imageDepth);

	const bool hasAlpha = CVTFFile::GetImageFormatInfo(vtf.GetFormat()).uiAlphaBitsPerPixel > 0;
	const VTFImageFormat format = hasAlpha && alpha ? IMAGE_FORMAT_RGBA8888 : IMAGE_FORMAT_RGB888;
	auto size = CVTFFile::ComputeMipmapSize(vtf.GetWidth(), vtf.GetHeight(), 1, mip, format);

	auto* imageData = new std::byte[size];
	bool ok = CVTFFile::Convert(vtf.GetData(frame, face, 0, mip), reinterpret_cast<vlByte*>(imageData), imageWidth, imageHeight, vtf.GetFormat(), format);
	if (!ok) {
		return std::nullopt;
	}
	return VTFData{
		std::unique_ptr<std::byte[]>{imageData},
		imageWidth,
		imageHeight,
		imageDepth,
		hasAlpha && alpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888
	};
}
