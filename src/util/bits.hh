#pragma once

/*
 * Misc. bitwise functions
 */

 //
 // Toggle the use of intrinsics w/ these macros
 //  Note: USE_BMI2 requires -mbmi2, USE_LSB/USE_POPCNT
 //        may need -msse or -mpopcnt.
 //  Another note: Don't use USE_BMI2 w/ an AMD CPU (it's extremely slow).
 //

// #define USE_BMI2
// #define USE_LSB
// #define USE_POPCNT

#if defined(_MSC_VER)
#	pragma warning(disable: 4146) // unary minus operator applied to unsigned type
#endif

#if defined(USE_BMI2)
#	include <immintrin.h>
#endif

#if defined(USE_LSB)
#	if defined(_MSC_VER)
#		include <intrin.h>
#	endif
#endif

#if defined(USE_POPCNT)
#	if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#		include <nmmintrin.h>
#	endif
#endif

#include "assert.hh"

namespace util
{

#if defined(USE_BMI2)
	constexpr bool HasBMI2 = true;

	inline std::uint64_t pext_64(std::uint64_t x, std::uint64_t mask)
	{
		return _pext_u64(x, mask);
	}

	inline std::uint64_t pdep_64(std::uint64_t x, std::uint64_t mask)
	{
		return _pdep_u64(x, mask);
	}
#else
	constexpr bool HasBMI2 = false;

	/**
	 * @brief 64-bit parallel bits extract from @param x using @param mask
	 * 
	 * @param x 
	 * @param mask 
	 * @return result
	 */
	constexpr std::uint64_t pext_64(std::uint64_t x, std::uint64_t mask)
	{
		std::uint64_t res = 0;

		for (std::uint64_t bb = 1; mask; bb += bb)
		{
			if (x & mask & -mask)
				res |= bb;

			mask &= (mask - 1);
		}

		return res;
	}

	/**
	 * @brief 64-bit parallel bits deposit from @param x using @param mask
	 * 
	 * @param x 
	 * @param mask 
	 * @return result 
	 */
	constexpr std::uint64_t pdep_64(std::uint64_t x, std::uint64_t mask)
	{
		std::uint64_t res = 0;

		for (std::uint64_t bb = 1; mask; bb += bb)
		{
			if (x & bb)
				res |= mask & (-mask);

			mask &= (mask - 1);
		}

		return res;
	}
#endif

#if defined(USE_LSB)
	constexpr bool HasLSBIntrinsics = true;

#	if defined(__GNUC__)
	inline unsigned lsb_64(std::uint64_t x)
	{
		ASSERT(x != 0);

		return __builtin_ctzll(x);
	}

	inline unsigned msb_64(std::uint64_t x)
	{
		ASSERT(x != 0);

		return __builtin_clzll(x) ^ 63u;
	}
#	elif defined(_MSC_VER)
	inline unsigned lsb_64(std::uint64_t x)
	{
		ASSERT(x != 0);

		unsigned long result;
		_BitScanForward64(&result, x);
		return static_cast<unsigned>(result);
	}

	inline unsigned msb_64(std::uint64_t x)
	{
		ASSERT(x != 0);

		unsigned long result;
		_BitScanReverse64(&result, x);
		return static_cast<unsigned>(result);
	}
#	else
#		error "LSB intrinsics not supported"
#	endif
#else
	constexpr bool HasLSBIntrinsics = false;

	/**
	 * @brief Calculates the least significant bit of an unsigned 64-bit integer @param x
	 * 
	 * @param x 
	 * @return Index of the least significant bit of @param x
	 */
	constexpr unsigned lsb_64(std::uint64_t x)
	{
		constexpr std::uint64_t lsb_magic_64 = 0x021937ec54bad1e7;
		constexpr unsigned lsb_index_64[]
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

	/**
	 * @brief Calculates the most significant bit of an unsigned 64-bit integer @param x
	 * 
	 * @param x 
	 * @return Index of the most significant bit of @param x
	 */
	constexpr unsigned msb_64(std::uint64_t x)
	{
		constexpr std::uint64_t msb_magic_64 = 0x03f79d71b4cb0a89;
		constexpr unsigned msb_index_64[]
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
#endif

#if defined(USE_POPCNT)
	constexpr bool HasPopcntIntrinsics = true;

#	if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	inline unsigned popcount_64(std::uint64_t x)
	{
		return static_cast<unsigned>(_mm_popcnt_u64(x));
	}
#	elif defined(__GNUC__)
	inline unsigned popcount_64(std::uint64_t x)
	{
		return __builtin_popcountll(x);
	}
#	else
#		error "POPCNT intrinsics not supported"
#	endif
#else
	constexpr bool HasPopcntIntrinsics = false;

	/**
	 * @brief Calculates the number of set bits in an unsigned 64-bit integer @param x
	 * 
	 * @param x 
	 * @return Number of set bits in @param x as an unsigned integer 
	 */
	constexpr unsigned popcount_64(std::uint64_t x)
	{
		constexpr std::uint64_t m1 = 0x5555555555555555ull;
		constexpr std::uint64_t m2 = 0x3333333333333333ull;
		constexpr std::uint64_t m4 = 0x0f0f0f0f0f0f0f0full;
		constexpr std::uint64_t h1 = 0x0101010101010101ull;

		x -= (x >> 1u) & m1;
		x = (x & m2) + ((x >> 2u) & m2);
		x = (x + (x >> 4u)) & m4;

		return (x * h1) >> 56u;
	}
#endif
}
