#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <span>
#include <utility>

#include <x86intrin.h>

namespace cdb {

constexpr auto byteswap(std::unsigned_integral auto value) {
  return [&]<auto... I>(std::index_sequence<I...>) {
    return ((((value >> I * 8) & 0xff) << ((8 * sizeof value) - ((1 + I) * 8))) | ...);
  } (std::make_index_sequence<sizeof value>());
}

constexpr auto popcount(std::unsigned_integral auto value) {
  return std::popcount(value);
}

constexpr auto lsb(std::unsigned_integral auto value) {
  return std::countr_zero(value);
}

constexpr auto msb(std::unsigned_integral auto value) {
  return std::countl_zero(value);
}

constexpr auto pext(std::uint64_t x, std::uint64_t mask) {
  if (std::is_constant_evaluated()) {
    std::uint64_t res = 0;

    for (std::uint64_t bb = 1; mask; bb += bb) {
      if (x & mask & -mask)
        res |= bb;

      mask &= (mask - 1);
    }

    return res;
  } else {
    return static_cast<std::uint64_t>(_pext_u64(x, mask));
  }
}

constexpr auto pdep(std::uint64_t x, std::uint64_t mask) {
  if (std::is_constant_evaluated()) {
    std::uint64_t res = 0;

    for (std::uint64_t bb = 1; mask; bb += bb) {
      if (x & bb)
        res |= mask & -mask;

      mask &= (mask - 1);
    }

    return res;
  } else {
    return static_cast<std::uint64_t>(_pdep_u64(x, mask));
  }
}

template <std::unsigned_integral T, std::size_t S>
constexpr auto read_le(std::span<std::uint8_t, S> data) {
  static_assert(sizeof(T) <= S);
  return [&]<auto... I>(std::index_sequence<I...>) {
    return ((static_cast<T>(data[I]) << (8 * (sizeof(T) - I - 1))) | ...);
  } (std::make_index_sequence<sizeof(T)>());
}

template <std::size_t S>
constexpr auto write_le(std::span<std::uint8_t, S> data, std::unsigned_integral auto uint) {
  static_assert(sizeof uint <= S);
  return [&]<auto... I>(std::index_sequence<I...>) {
    ((data[I] = ((uint >> (8 * I) & 0xff))), ...);
  } (std::make_index_sequence<sizeof uint>());
}

} // cdb
