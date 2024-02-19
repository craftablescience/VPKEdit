#pragma once

#include <QWidget>

namespace ThemedIcon {

QPixmap get(const QWidget* parent, const QString& path, QPalette::ColorRole role);

} // namespace ThemedIcon
