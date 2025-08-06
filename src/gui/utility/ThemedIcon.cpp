#include "ThemedIcon.h"

#include <QStyleOption>

QPixmap ThemedIcon::get(const QWidget* parent, const QString& path, QPalette::ColorRole role) {
	QStyleOption opt;
	opt.initFrom(parent);

	QPixmap pixmap{path};
	const auto mask = pixmap.createMaskFromColor(Qt::white, Qt::MaskOutColor);
	pixmap.fill(opt.palette.color(role));
	pixmap.setMask(mask);
	return pixmap;
}
