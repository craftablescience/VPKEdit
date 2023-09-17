#include <vpkedit/detail/CRC.h>

using namespace vpkedit;

unsigned int detail::computeCRC(const std::vector<std::byte>& buffer) {
    unsigned int crc = 0xffffffff;
    for (auto byte : buffer) {
        crc = (crc >> 8) ^ CRC_TABLE[static_cast<unsigned int>(byte) ^ crc & 0xff];
    }
    return ~crc;
}
