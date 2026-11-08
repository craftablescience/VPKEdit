#include "VTFThumbnail.hpp"

#include <QPainter>
#include <vtfpp/VTF.h>

// Code in this block modified from the MareTF thumbnailer. Thanks, Laura!!! Love you 💙
namespace MareTF {

namespace {

[[nodiscard]] std::vector<std::byte> drawCubemapNetCentered(int& faceWidth, int& faceHeight, const QImage& xp, const QImage& xn, const QImage& yp, const QImage& yn, const QImage& zp, const QImage& zn, const QImage& sm) {
	QImage net{faceWidth * 4, faceHeight * 3, QImage::Format_ARGB32_Premultiplied};
	net.fill(Qt::transparent);

	QPainter painter{&net};
	painter.drawImage(faceWidth * 1, faceHeight * 0, yp);
	painter.drawImage(faceWidth * 0, faceHeight * 1, xn);
	painter.drawImage(faceWidth * 1, faceHeight * 1, zp);
	painter.drawImage(faceWidth * 2, faceHeight * 1, xp);
	painter.drawImage(faceWidth * 3, faceHeight * 1, zn);
	painter.drawImage(faceWidth * 1, faceHeight * 2, yn);
	if (!sm.isNull()) {
		painter.drawImage(faceWidth * 2, faceHeight * 2, sm);
	}
	painter.end();

	faceWidth *= 4;
	faceHeight *= 3;

	return {reinterpret_cast<const std::byte*>(net.constBits()), reinterpret_cast<const std::byte*>(net.constBits()) + net.sizeInBytes()};
}

std::vector<std::byte> createThumbnail(const vtfpp::VTF& vtf, int& width, int& height) {
	if (!vtf) {
		return {};
	}

	width = vtf.getWidth();
	height = vtf.getHeight();

	if (vtf.getFaceCount() >= 6) {
		// Cubemap
		// Ignore alpha because it's unused uninitialized memory
		const auto xpData = vtf.getImageDataAs(vtfpp::ImageFormat::RGB888, 0, 0, 0);
		const auto xnData = vtf.getImageDataAs(vtfpp::ImageFormat::RGB888, 0, 0, 1);
		const auto ypData = vtf.getImageDataAs(vtfpp::ImageFormat::RGB888, 0, 0, 2);
		const auto ynData = vtf.getImageDataAs(vtfpp::ImageFormat::RGB888, 0, 0, 3);
		const auto zpData = vtf.getImageDataAs(vtfpp::ImageFormat::RGB888, 0, 0, 4);
		const auto znData = vtf.getImageDataAs(vtfpp::ImageFormat::RGB888, 0, 0, 5);
		const auto smData = vtf.getFaceCount() > 6 ? vtf.getImageDataAs(vtfpp::ImageFormat::RGB888, 0, 0, 6) : std::vector<std::byte>{};

		const QImage xp{reinterpret_cast<const uchar*>(xpData.data()), width, height, QImage::Format_RGB888};
		const QImage xn{reinterpret_cast<const uchar*>(xnData.data()), width, height, QImage::Format_RGB888};
		const QImage yp{reinterpret_cast<const uchar*>(ypData.data()), width, height, QImage::Format_RGB888};
		const QImage yn{reinterpret_cast<const uchar*>(ynData.data()), width, height, QImage::Format_RGB888};
		const QImage zp{reinterpret_cast<const uchar*>(zpData.data()), width, height, QImage::Format_RGB888};
		const QImage zn{reinterpret_cast<const uchar*>(znData.data()), width, height, QImage::Format_RGB888};
		const auto sm = !smData.empty() ? QImage{reinterpret_cast<const uchar*>(smData.data()), width, height, QImage::Format_RGB888} : QImage{};

		return drawCubemapNetCentered(width, height, xp, xn, yp, yn, zp, zn, sm);
	}
	if (vtf.getFlags() & vtfpp::VTF::FLAG_V0_NORMAL || (vtf.getVersion() >= 3 && vtf.getFlags() & vtfpp::VTF::FLAG_V3_SSBUMP)) {
		// Regular texture where alpha is a mask
		return vtfpp::ImageConversion::convertImageDataToFormat(vtf.getImageDataAs(vtfpp::ImageFormat::RGB888), vtfpp::ImageFormat::RGB888, vtfpp::ImageFormat::BGRA8888, width, height);
	}
	// Regular texture where alpha is alpha
	return vtfpp::ImageConversion::convertImageDataToFormat(vtf.getImageDataAsRGBA8888(), vtfpp::ImageFormat::BGRA8888, vtfpp::ImageFormat::RGBA8888, width, height);
}

} // namespace

} // namespace MareTF

void VTFThumbnail::initPlugin(VWindowAccess_V6_0*) {}

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
	int width = 0, height = 0;
	if (auto imageData = MareTF::createThumbnail(vtfpp::VTF{{reinterpret_cast<const std::byte*>(dataPtr), dataLen}}, width, height); !imageData.empty() && width > 0 && height > 0) {
		return QPixmap::fromImage(QImage{reinterpret_cast<uchar*>(imageData.data()), width, height, QImage::Format_ARGB32});
	}
	return {};
}
