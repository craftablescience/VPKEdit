#include <vpkedit/detail/MD5.h>

#include <MD5.h>

using namespace vpkedit;

std::array<std::byte, 16> detail::computeMD5(const std::vector<std::byte>& buffer) {
	return md5(buffer);
}

std::array<std::byte, 16> detail::computeMD5(const std::byte* buffer, std::size_t len) {
	return md5(buffer, len);
}
