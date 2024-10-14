#pragma once

#include <QImage>

namespace ImageLoader {

[[nodiscard]] QImage load(const QString& imagePath);

} // namespace ImageLoader
