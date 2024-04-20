#include <vpkedit/detail/Adler32.h>

#include <cryptopp/adler32.h>

using namespace vpkedit;

std::uint32_t detail::computeAdler32(const std::vector<std::byte>& buffer) {
	return computeAdler32(buffer.data(), buffer.size());
}

std::uint32_t detail::computeAdler32(const std::byte* buffer, std::size_t len) {
	// Make sure this is right
	static_assert(CryptoPP::Adler32::DIGESTSIZE == sizeof(std::uint32_t));

	CryptoPP::Adler32 adler32;
	adler32.Update(reinterpret_cast<const CryptoPP::byte*>(buffer), len);

	std::uint32_t final{};
	adler32.Final(reinterpret_cast<CryptoPP::byte*>(&final));
	return final;
}
