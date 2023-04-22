/**
 * komihash.h version 4.7
 *
 * The inclusion file for the "komihash" hash function, "komirand" 64-bit
 * PRNG, and streamed "komihash" implementation.
 *
 * Description is available at https://github.com/avaneev/komihash
 *
 * License
 *
 * Copyright (c) 2021-2022 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

//------------------------------------------------------------

#if defined(__GNUC__) || defined(__clang__)
  #define KOMIHASH_PREFETCH(addr) do { \
    if (!std::is_constant_evaluated()) \
      __builtin_prefetch(addr, 0, 1);  \
  } while (0)
#else
  #define KOMIHASH_PREFETCH(addr)
#endif

template <std::unsigned_integral T>
constexpr auto kh_luec(const std::uint8_t * const data) -> T {
  return [&]<auto... I>(std::index_sequence<I...>) {
    return ((static_cast<T>(data[I]) << (8 * I)) | ...);
  } (std::make_index_sequence<sizeof(T)>());
}

constexpr auto kh_lu64ec = kh_luec<uint64_t>;
constexpr auto kh_lu32ec = kh_luec<uint32_t>;

constexpr uint64_t kh_lpu64ec_l3(const uint8_t * const msg, const size_t size) {
  const int ml8 = -(int)(size * 8);

  if (size < 4) {
    const uint8_t * const msg3 = msg + size - 3;
    const uint64_t m = (uint64_t)msg3[0] | (uint64_t)msg3[1] << 8 | (uint64_t)msg3[2] << 16;

    return 1ULL << ((msg3[2] >> 7) - ml8) | m >> (24 + ml8);
  }

  const uint64_t mh = kh_lu32ec(msg + size - 4);
  const uint64_t ml = kh_lu32ec(msg);

  return 1ULL << ((int)(mh >> 31) - ml8) | ml | (mh >> (64 + ml8)) << 32;
}

constexpr uint64_t kh_lpu64ec_nz(const uint8_t * const msg, const size_t size) {
  const int ml8 = -(int)(size * 8);

  if (size < 4) {
    uint64_t m = msg[0];
    const uint8_t mf = msg[size - 1];

    if (size > 1) {
      m |= (uint64_t)msg[1] << 8;

      if (size > 2)
        m |= (uint64_t)mf << 16;
    }

    return 1ULL << ((mf >> 7) - ml8) | m;
  }

  const uint64_t mh = kh_lu32ec(msg + size - 4);
  const uint64_t ml = kh_lu32ec(msg);

  return 1ULL << ((int)(mh >> 31) - ml8) | ml | (mh >> (64 + ml8)) << 32;
}

constexpr uint64_t kh_lpu64ec_l4(const uint8_t * const msg, const size_t size) {
  const int ml8 = -(int)(size * 8);

  if (size < 5) {
    const uint64_t m = kh_lu32ec(msg + size - 4);
    return 1ULL << ((int)(m >> 31) - ml8) | m >> (32 + ml8);
  }

  const uint64_t m = kh_lu64ec(msg + size - 8);
  return 1ULL << ((int)(m >> 63) - ml8) | m >> (64 + ml8);
}

#if defined(__SIZEOF_INT128__)
constexpr void kh_m128(const uint64_t m1, const uint64_t m2,
                       uint64_t * const rl, uint64_t * const rh) {
  const __uint128_t r = (__uint128_t)m1 * m2;

  *rl = (uint64_t)(r);
  *rh = (uint64_t)(r >> 64);
}

#elif defined( _MSC_VER ) && defined( _M_X64 )
#include <intrin.h>

inline void kh_m128(const uint64_t m1, const uint64_t m2,
                    uint64_t * const rl, uint64_t * const rh) {
  *rl = _umul128(m1, m2, rh);
}
#else
// _umul128() code for 32-bit systems, adapted from mullu(),
// from https://go.dev/src/runtime/softfloat64.go
// Licensed under BSD-style license.

constexpr uint64_t kh__emulu(const uint32_t x, const uint32_t y) {
  return( x * (uint64_t) y );
}

constexpr void kh_m128(const uint64_t u, const uint64_t v,
                       uint64_t * const rl, uint64_t * const rh) {
  *rl = u * v;

  const uint32_t u0 = (uint32_t)u;
  const uint32_t v0 = (uint32_t)v;
  const uint64_t w0 = kh__emulu(u0, v0);
  const uint32_t u1 = (uint32_t)(u >> 32);
  const uint32_t v1 = (uint32_t)(v >> 32);
  const uint64_t t0 = kh__emulu(u1, v0) + (w0 >> 32);
  const uint64_t w1 = (uint32_t)t0 + kh__emulu(u0, v1);

  *rh = kh__emulu(u1, v1) + (w1 >> 32) + (t >> 32);
}
#endif // _MSC_VER

#define KOMIHASH_HASH16(m)                                               \
  kh_m128(seed1 ^ kh_lu64ec(m), seed5 ^ kh_lu64ec(m + 8), &seed1, &r1h); \
  seed5 += r1h;                                                          \
  seed1 ^= seed5;

#define KOMIHASH_HASHROUND()           \
  kh_m128(seed1, seed5, &seed1, &r2h); \
  seed5 += r2h;                        \
  seed1 ^= seed5;

#define KOMIHASH_HASHFIN()         \
  kh_m128(r1h, r2h, &seed1, &r1h); \
  seed5 += r1h;                    \
  seed1 ^= seed5;                  \
  KOMIHASH_HASHROUND();            \
  return seed1;

#define KOMIHASH_HASHLOOP64()                           \
  do {                                                  \
    KOMIHASH_PREFETCH(msg);                             \
    kh_m128(seed1 ^ kh_lu64ec(msg),                     \
            seed5 ^ kh_lu64ec(msg + 8),  &seed1, &r1h); \
    kh_m128(seed2 ^ kh_lu64ec(msg + 16),                \
            seed6 ^ kh_lu64ec(msg + 24), &seed2, &r2h); \
    kh_m128(seed3 ^ kh_lu64ec(msg + 32),                \
            seed7 ^ kh_lu64ec(msg + 40), &seed3, &r3h); \
    kh_m128(seed4 ^ kh_lu64ec(msg + 48),                \
            seed8 ^ kh_lu64ec(msg + 56), &seed4, &r4h); \
    size -= 64;                                         \
    msg += 64;                                          \
    seed5 += r1h;                                       \
    seed6 += r2h;                                       \
    seed7 += r3h;                                       \
    seed8 += r4h;                                       \
    seed2 ^= seed5;                                     \
    seed3 ^= seed6;                                     \
    seed4 ^= seed7;                                     \
    seed1 ^= seed8;                                     \
  } while (size > 63); [[likely]]


inline uint64_t komihash_epi(const uint8_t *msg, size_t size, uint64_t seed1, uint64_t seed5) {
  uint64_t r1h, r2h;

  KOMIHASH_PREFETCH(msg);

  if (size > 31) [[likely]] {
    KOMIHASH_HASH16(msg);
    KOMIHASH_HASH16(msg + 16);

    msg += 32;
    size -= 32;
  }

  if (size > 15) {
    KOMIHASH_HASH16(msg);

    msg += 16;
    size -= 16;
  }

  if (size > 7) {
    r2h = seed5 ^ kh_lpu64ec_l4(msg + 8, size - 8);
    r1h = seed1 ^ kh_lu64ec(msg);
  } else {
    r1h = seed1 ^ kh_lpu64ec_l4(msg, size);
    r2h = seed5;
  }

  KOMIHASH_HASHFIN();
}

inline uint64_t komihash(const uint8_t * const msg0, size_t size, const uint64_t seed) {
  const uint8_t *msg = (const uint8_t *)msg0;

  uint64_t seed1 = 0x243F6A8885A308D3 ^ (seed & 0x5555555555555555);
  uint64_t seed5 = 0x452821E638D01377 ^ (seed & 0xAAAAAAAAAAAAAAAA);
  uint64_t r1h, r2h;

  KOMIHASH_HASHROUND();

  if (size < 16) [[likely]] {
    KOMIHASH_PREFETCH(msg);

    r1h = seed1;
    r2h = seed5;

    if (size > 7) {
      r2h ^= kh_lpu64ec_l3(msg + 8, size - 8);
      r1h ^= kh_lu64ec(msg);
    } else if(size > 0) [[likely]]
      r1h ^= kh_lpu64ec_nz(msg, size);

    KOMIHASH_HASHFIN();
  }

  if (size < 32) [[likely]] {
    KOMIHASH_PREFETCH(msg);

    KOMIHASH_HASH16(msg);

    if(size > 23) {
      r2h = seed5 ^ kh_lpu64ec_l4(msg + 24, size - 24);
      r1h = seed1 ^ kh_lu64ec(msg + 16);
    } else {
      r1h = seed1 ^ kh_lpu64ec_l4(msg + 16, size - 16);
      r2h = seed5;
    }

    KOMIHASH_HASHFIN();
  }

  if (size > 63) {
    uint64_t seed2 = 0x13198A2E03707344 ^ seed1;
    uint64_t seed3 = 0xA4093822299F31D0 ^ seed1;
    uint64_t seed4 = 0x082EFA98EC4E6C89 ^ seed1;
    uint64_t seed6 = 0xBE5466CF34E90C6C ^ seed5;
    uint64_t seed7 = 0xC0AC29B7C97C50DD ^ seed5;
    uint64_t seed8 = 0x3F84D5B5B5470917 ^ seed5;
    uint64_t r3h, r4h;

    KOMIHASH_HASHLOOP64();

    seed5 ^= seed6 ^ seed7 ^ seed8;
    seed1 ^= seed2 ^ seed3 ^ seed4;
  }

  return komihash_epi(msg, size, seed1, seed5);
}

template <std::size_t S>
static inline uint64_t komihash(std::span<const std::byte, S> msg, const uint64_t seed) {
  return komihash((const uint8_t *)msg.data(), msg.size(), seed);
}

template <std::size_t S>
static inline uint64_t komihash(std::span<std::byte, S> msg, const uint64_t seed) {
  return komihash((const uint8_t *)msg.data(), msg.size(), seed);
}

static inline uint64_t komihashv(const void * const msg0, size_t size, const uint64_t seed) {
  return komihash((const uint8_t *)msg0, size, seed);
}
