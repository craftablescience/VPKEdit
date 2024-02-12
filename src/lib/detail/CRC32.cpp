#include <vpkedit/detail/CRC32.h>

using namespace vpkedit;

std::uint32_t detail::computeCRC(const std::vector<std::byte>& buffer) {
    return computeCRC(buffer.data(), buffer.size());
}

std::uint32_t detail::computeCRC(const std::byte* buffer, std::size_t len) {
    unsigned int crc = 0xffffffff;
    for (std::size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ CRC_TABLE[static_cast<unsigned int>(buffer[i]) ^ crc & 0xff];
    }
    return ~crc;
}
