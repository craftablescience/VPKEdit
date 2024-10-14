#pragma once

#include <cstddef>
#include <vector>

#include <QImage>

namespace ImageLoader {

[[nodiscard]] QImage load(const QString& imagePath);

[[nodiscard]] QImage load(const std::vector<std::byte>& imageData);

} // namespace ImageLoader
