#pragma once

#if defined(__cplusplus)
extern "C" {
#else
#	include <stdbool.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__PRETTY_FUNCTION__)
#	define CUR_FUNC __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#	define CUR_FUNC __FUNCSIG__
#else
#	define CUR_FUNC __FUNCTION__
#endif

#define CUR_FILE __FILE__
#define CUR_LINE __LINE__

static inline int assertion_failed(const char *func, const char *file,
								   int line, const char *cond)
{
	fprintf(stderr, "%s:%03d %s : expected '%s'\n", file, line, func, cond);
	abort();
	return 0;
}

#if defined(NDEBUG)
#	define ASSERT(cond) (void)(0)
#else
#	define ASSERT(cond) (void)(!(cond) ? assertion_failed(CUR_FUNC, CUR_FILE, CUR_LINE, #cond) : 0)
#endif

// #define USE_INTRINSICS

#if defined(USE_INTRINSICS)
#	include <x86intrin.h>
#endif

#if defined(USE_INTRINSICS) && defined(__BMI2__)
#define HAS_BMI2

static inline uint64_t pext(uint64_t x, uint64_t mask)
{
	return _pext_u64(x, mask);
}

static inline uint64_t pdep(uint64_t x, uint64_t mask)
{
	return _pdep_u64(x, mask);
}
#else
static inline uint64_t pext(uint64_t x, uint64_t mask)
{
	uint64_t res = 0;

	for (uint64_t bb = 1; mask; bb += bb)
	{
		if (x & mask & -mask)
			res |= bb;

		mask &= (mask - 1);
	}

	return res;
}

static inline uint64_t pdep(uint64_t x, uint64_t mask)
{
	uint64_t res = 0;

	for (uint64_t bb = 1; mask; bb += bb)
	{
		if (x & bb)
			res |= mask & (-mask);

		mask &= (mask - 1);
	}

	return res;
}
#endif // USE_INTRINSICS && __BMI2__

#if defined(USE_INTRINSICS)
#define HAS_LSB

#	if defined(__GNUC__)
static inline unsigned lsb(uint64_t x)
{
	ASSERT(x != 0);
	return __builtin_ctzll(x);
}

static inline unsigned msb(uint64_t x)
{
	ASSERT(x != 0);
	return __builtin_clzll(x) ^ 63u;
}
#	elif defined(_MSC_VER)
static inline unsigned lsb(uint64_t x)
{
	ASSERT(x != 0);

	unsigned long result;
	_BitScanForward64(&result, x);
	return result;
}

static inline unsigned msb(uint64_t x)
{
	ASSERT(x != 0);

	unsigned long result;
	_BitScanReverse64(&result, x);
	return result;
}
#	else
#		error "LSB intrinsics not supported"
#	endif
#else
static inline unsigned lsb(uint64_t x)
{
	static const uint64_t lsb_magic_64 = 0x021937ec54bad1e7;
	static const unsigned lsb_index_64[] =
	{
		0,  1,	2,  7,  3, 30,  8,  52,
		4,  13, 31, 38, 9,  16, 59, 53,
		5,  50, 36, 14, 34, 32, 45, 39,
		27, 10, 47, 17, 60, 41, 54, 20,
		63, 6,  29, 51, 12, 37, 15, 58,
		49, 35, 33, 44, 26, 46, 40, 19,
		62, 28, 11, 57, 48, 43, 25, 18,
		61, 56, 42, 24, 55, 23, 22, 21
	};

	return lsb_index_64[((x & (~x + 1)) * lsb_magic_64) >> 58u];
}

static inline unsigned msb(uint64_t x)
{
	static const uint64_t msb_magic_64 = 0x03f79d71b4cb0a89;
	static const unsigned msb_index_64[] =
	{
		0,  47, 1,  56, 48, 27, 2,  60,
		57, 49, 41, 37, 28, 16, 3,  61,
		54, 58, 35, 52, 50, 42, 21, 44,
		38, 32, 29, 23, 17, 11, 4,  62,
		46, 55, 26, 59, 40, 36, 15, 53,
		34, 51, 20, 43, 31, 22, 10, 45,
		25, 39, 14, 33, 19, 30, 9,  24,
		13, 18, 8,  12, 7,  6,  5,  63
	};

	x |= x >> 1u;
	x |= x >> 2u;
	x |= x >> 4u;
	x |= x >> 8u;
	x |= x >> 16u;
	x |= x >> 32u;

	return msb_index_64[(x * msb_magic_64) >> 58u];
}
#endif // USE_INTRINSICS

#if defined(USE_INTRINSICS)
#define HAS_POPCNT

static inline unsigned popcnt(uint64_t x)
{
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	return _mm_popcnt_u64(x);
#elif defined(__GNUC__)
	return __builtin_popcountll(x);
#else
#	error "POPCNT intrinsics not supported"
#endif
}
#else
static inline unsigned popcnt(uint64_t x)
{
	static const uint64_t m1 = 0x5555555555555555ull;
	static const uint64_t m2 = 0x3333333333333333ull;
	static const uint64_t m4 = 0x0f0f0f0f0f0f0f0full;
	static const uint64_t h1 = 0x0101010101010101ull;

	x -= (x >> 1u) & m1;
	x = (x & m2) + ((x >> 2u) & m2);
	x = (x + (x >> 4u)) & m4;

	return (x * h1) >> 56u;
}
#endif // USE_INTRINSICS

#if defined(USE_INTRINSICS)
#define HAS_BSWAP

static inline uint64_t bswap(uint64_t x)
{
#if defined(__GNUC__)
	return __builtin_bswap64(x);
#elif defined(_MSC_VER)
	return _byteswap_uint64(x);
#else
#	error "Not supported"
#endif
}
#else
#	error "Not implemented"
#endif // USE_INTRINSICS

#if defined(__cplusplus)
} // extern "C"
#endif
