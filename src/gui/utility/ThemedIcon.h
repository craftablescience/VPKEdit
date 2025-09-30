#pragma once

#include <QStyleOption>

namespace ThemedIcon {

inline QPixmap get(const QWidget* parent, const QString& path, QPalette::ColorRole role) {
	QStyleOption opt;
	opt.initFrom(parent);

	QPixmap pixmap{path};
	const auto mask = pixmap.createMaskFromColor(Qt::white, Qt::MaskOutColor);
	pixmap.fill(opt.palette.color(role));
	pixmap.setMask(mask);
	return pixmap;
}

} // namespace ThemedIcon
