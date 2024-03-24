#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace vpkedit::detail {

std::array<std::byte, 16> computeMD5(const std::vector<std::byte>& buffer);

std::array<std::byte, 16> computeMD5(const std::byte* buffer, std::size_t len);

} // namespace vpkedit::detail
