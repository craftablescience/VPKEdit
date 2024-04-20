#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace vpkedit::detail {

std::pair<std::string, std::string> computeSHA256KeyPair(std::uint16_t size = 2048);

bool verifySHA256Key(const std::vector<std::byte>& buffer, const std::vector<std::byte>& publicKey, const std::vector<std::byte>& signature);

std::vector<std::byte> signDataWithSHA256Key(const std::vector<std::byte>& buffer, const std::vector<std::byte>& privateKey);

std::vector<std::byte> decodeHexString(std::string_view hex);

} // namespace vpkedit::detail
