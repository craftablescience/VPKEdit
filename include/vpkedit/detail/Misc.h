#pragma once

#include <cstddef>
#include <string>
#include <tuple>
#include <vector>

namespace vpkedit::detail {

void toLowerCase(std::string& input);

void normalizeSlashes(std::string& path);

std::pair<std::string, std::string> splitFilenameAndParentDir(const std::string& filename);

std::vector<std::byte> readFileData(const std::string& filepath, std::size_t preloadBytesOffset);

} // namespace vpkedit::detail
