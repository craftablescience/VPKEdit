#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vpkedit::detail {

std::uint32_t computeAdler32(const std::vector<std::byte>& buffer);

std::uint32_t computeAdler32(const std::byte* buffer, std::size_t len);

} // namespace vpkedit::detail
