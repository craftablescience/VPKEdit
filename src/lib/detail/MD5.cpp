#include <vpkedit/detail/MD5.h>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>

using namespace vpkedit;

std::array<std::byte, 16> detail::computeMD5(const std::vector<std::byte>& buffer) {
	return computeMD5(buffer.data(), buffer.size());
}

std::array<std::byte, 16> detail::computeMD5(const std::byte* buffer, std::size_t len) {
	// Make sure this is right
	static_assert(CryptoPP::Weak::MD5::DIGESTSIZE == sizeof(std::array<std::byte, 16>));

	CryptoPP::Weak::MD5 md5;
	md5.Update(reinterpret_cast<const CryptoPP::byte*>(buffer), len);

	std::array<std::byte, 16> final{};
	md5.Final(reinterpret_cast<CryptoPP::byte*>(final.data()));
	return final;
}
