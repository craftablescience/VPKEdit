#pragma once

#include <optional>

#include <QIcon>

namespace TGADecoder {

std::optional<QImage> decodeImage(const QString& imagePath);

} // namespace TGADecoder
