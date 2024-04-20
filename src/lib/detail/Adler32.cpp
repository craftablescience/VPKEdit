/*
 * Copyright notice:
 *
 * (C) 1995-2024 Jean-loup Gailly and Mark Adler
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Jean-loup Gailly        Mark Adler
 * jloup@gzip.org          madler@alumni.caltech.edu
 */

#include <vpkedit/detail/Adler32.h>

using namespace vpkedit;

constexpr std::uint32_t BASE = 65521u;    /* largest prime smaller than 65536 */
constexpr std::size_t NMAX = 5552u;

#define DO1(buffer,i)  {adler += static_cast<unsigned char>((buffer)[i]); sum2 += adler;}
#define DO2(buffer,i)  DO1(buffer,i) DO1(buffer,i+1)
#define DO4(buffer,i)  DO2(buffer,i) DO2(buffer,i+2)
#define DO8(buffer,i)  DO4(buffer,i) DO4(buffer,i+4)
#define DO16(buffer)   DO8(buffer,0) DO8(buffer,8)

std::uint32_t detail::computeAdler32(const std::vector<std::byte>& buffer, std::uint32_t adler) {
	return computeAdler32(buffer.data(), buffer.size(), adler);
}

std::uint32_t detail::computeAdler32(const std::byte* buffer, std::size_t len, std::uint32_t adler) {
	std::uint32_t sum2;
	std::uint32_t n;

	/* split Adler-32 into component sums */
	sum2 = (adler >> 16) & 0xffff;
	adler &= 0xffff;

	/* in case user likes doing a byte at a time, keep it fast */
	if (len == 1) {
		adler += static_cast<unsigned char>(buffer[0]);
		if (adler >= BASE)
			adler -= BASE;
		sum2 += adler;
		if (sum2 >= BASE)
			sum2 -= BASE;
		return adler | (sum2 << 16);
	}

	/* initial Adler-32 value (deferred check for len == 1 speed) */
	if (!buffer)
		return 1L;

	/* in case short lengths are provided, keep it somewhat fast */
	if (len < 16) {
		while (len--) {
			adler += static_cast<unsigned char>(*buffer++);
			sum2 += adler;
		}
		if (adler >= BASE)
			adler -= BASE;
		sum2 %= BASE;            /* only added so many BASE's */
		return adler | (sum2 << 16);
	}

	/* do length NMAX blocks -- requires just one modulo operation */
	while (len >= NMAX) {
		len -= NMAX;
		n = NMAX / 16;          /* NMAX is divisible by 16 */
		do {
			DO16(buffer)           /* 16 sums unrolled */
			buffer += 16;
		} while (--n);
		adler %= BASE;
		sum2 %= BASE;
	}

	/* do remaining bytes (less than NMAX, still just one modulo) */
	if (len) {                  /* avoid modulos if none remaining */
		while (len >= 16) {
			len -= 16;
			DO16(buffer)
			buffer += 16;
		}
		while (len--) {
			adler += static_cast<unsigned char>(*buffer++);
			sum2 += adler;
		}
		adler %= BASE;
		sum2 %= BASE;
	}

	/* return recombined sums */
	return adler | (sum2 << 16);
}
