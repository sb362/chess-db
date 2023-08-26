#pragma once

#include <cstdint>
#include <cstring> // memcpy

#if defined(_MSC_VER) && defined(_M_X64)
#  include <intrin.h>
#  pragma intrinsic(_umul128)
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
#  define WYHASH_LIKELY(x)  __builtin_expect(x, 1)
#  define WYHASH_UNLIKELY(x)  __builtin_expect(x, 0)
#else
#  define WYHASH_LIKELY(x) (x)
#  define WYHASH_UNLIKELY(x) (x)
#endif

namespace wyh {

// Stripped down version of wyhash (77e50f26)
//   src: https://github.com/wangyi-fudan/wyhash
//   author: Wang Yi <godspeed_china@yeah.net>
//   license: unlicense (public domain)

static inline void mum(uint64_t *A, uint64_t *B) {
#if defined(__SIZEOF_INT128__)
  __uint128_t r = *A; r *= *B;
  *A ^= (uint64_t)(r);
  *B ^= (uint64_t)(r >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
  uint64_t a, b;
  a = _umul128(*A, *B, &b);
  *A ^= a;
  *B ^= b;
#else
  uint64_t ha = *A >> 32;
  uint64_t hb = *B >> 32;
  uint64_t la = (uint32_t)*A;
  uint64_t lb = (uint32_t)*B;
  uint64_t hi, lo;
  uint64_t rm0 = ha * lb;
  uint64_t rm1 = hb * la;
  uint64_t rh = ha * hb;
  uint64_t rl = la * lb;
  uint64_t t = rl + (rm0 << 32);
  uint64_t c = t < rl;
  lo = t + (rm1 << 32);
  c += lo < t;
  hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
  *A ^= lo;
  *B ^= hi;
#endif
}

// multiply and xor mix function
static inline uint64_t mix(uint64_t A, uint64_t B) {
  mum(&A, &B);
  return A ^ B;
}

static inline uint64_t r8(const uint8_t *p) {
  uint64_t v;
  memcpy(&v, p, 8);
  return v;
}

static inline uint64_t r4(const uint8_t *p) {
  uint32_t v;
  memcpy(&v, p, 4);
  return v;
}

static inline uint64_t r3(const uint8_t *p, size_t k) {
  return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1];
}

static constexpr uint64_t secret[4] = {0xa0761d6478bd642full, 0xe7037ed1a0b428dbull,
                                       0x8ebc6af09c88c6e3ull, 0x589965cc75374cc3ull};

static inline uint64_t wyhash(const void *key, size_t len, uint64_t seed = 0) {
  auto p = (const uint8_t *)key;
  seed ^= mix(seed ^ secret[0], secret[1]);
  uint64_t a, b;

  if (WYHASH_LIKELY(len <= 16)) {
    if (WYHASH_LIKELY(len >= 4)) {
      a = (r4(p) << 32) | r4(p + ((len >> 3) << 2));
      b = (r4(p + len - 4) << 32) | r4(p + len - 4 - ((len >> 3) << 2));
    } else if (WYHASH_LIKELY(len > 0)) {
      a = r3(p, len);
      b = 0;
    } else
      a = b = 0;
  } else {
    size_t i = len;
    if (WYHASH_UNLIKELY(i > 48)) {
      uint64_t s1 = seed, s2 = seed;
      do {
        seed = mix(r8(p) ^ secret[1], r8(p + 8) ^ seed);
        s1 = mix(r8(p + 16) ^ secret[2], r8(p + 24) ^ s1);
        s2 = mix(r8(p + 32) ^ secret[3], r8(p + 40) ^ s2);
        p += 48;
        i -= 48;
      } while (WYHASH_LIKELY(i > 48));
      seed ^= s1 ^s2;
    } while (WYHASH_UNLIKELY(i > 16)) {
      seed = mix(r8(p) ^ secret[1], r8(p + 8) ^ seed);
      i -= 16;
      p += 16;
    }
    a = r8(p + i - 16);
    b = r8(p + i - 8);
  }

  a ^= secret[1];
  b ^= seed;
  mum(&a, &b);
  return mix(a ^ secret[0] ^ len, b ^ secret[1]);
}

static constexpr uint64_t phi = 0x9e3779b97f4a7c15ull; // ≈ floor(2^64 / ϕ)

static inline uint64_t wyhash(uint64_t key) {
  key += secret[0];
  return mix(key, phi);
}

// 64 bit - 64 bit mix function
static inline uint64_t wymix(uint64_t A, uint64_t B) {
  A ^= secret[0];
  B ^= secret[1];
  mum(&A, &B);
  return mix(A ^ secret[0], B ^ secret[1]);
}

// PRNG
static inline uint64_t wyrand(uint64_t &seed) {
  seed += secret[0];
  return mix(seed, seed ^ secret[1]);
}

} // wyh
