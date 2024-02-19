#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
	#define VPKEDIT_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
	#define EXPORT  __attribute__((__visibility__("default")))
#else
	#define EXPORT
#endif

#ifdef __cplusplus
	#define VPKEDIT_API extern "C" VPKEDIT_EXPORT
	#include <cstddef>
	#include <cstdint>
	using std::size_t;
	using std::uint64_t;
	using std::uint32_t;
	using std::uint16_t;
#else
	#define VPKEDIT_API VPKEDIT_EXPORT
	#include <stddef.h>
	#include <stdint.h>
#endif
