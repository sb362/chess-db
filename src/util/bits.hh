#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <span>
#include <utility>

#ifdef _MSC_VER
//#include <intrin.h>
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace cdb {

constexpr std::uint32_t packed32(std::uint8_t x) {
  return x * 0x1010101;
}

constexpr std::uint64_t packed64(std::uint8_t x) {
  return x * 0x101010101010101;
}

constexpr auto byteswap(std::unsigned_integral auto value) -> decltype(value) {
  return [&]<auto... I>(std::index_sequence<I...>) {
    return ((((value >> I * 8) & 0xff) << ((8 * sizeof value) - ((1 + I) * 8))) | ...);
  } (std::make_index_sequence<sizeof value>());
}

constexpr int popcount(std::unsigned_integral auto value) {
  return std::popcount(value);
}

constexpr int lsb(std::unsigned_integral auto value) {
  return std::countr_zero(value);
}

constexpr int msb(std::unsigned_integral auto value) {
  return std::countl_zero(value);
}

constexpr auto pext(std::uint64_t x, std::uint64_t mask) -> std::uint64_t {
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

constexpr auto pdep(std::uint64_t x, std::uint64_t mask) -> std::uint64_t {
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

template <unsigned B>
using uint_t = std::tuple_element_t<(B > 1 + B > 2 + B > 4),
                                    std::tuple<std::uint8_t, std::uint16_t,
                                               std::uint32_t, std::uint64_t>>;

template <std::unsigned_integral T, std::size_t S, std::size_t B = sizeof(T)>
constexpr auto read_le(std::span<const std::byte, S> data, std::size_t offset = 0) -> T {
  static_assert(B <= S);
  static_assert(B <= sizeof(T));

  return [&]<auto... I>(std::index_sequence<I...>) {
    return ((static_cast<T>(data[offset + I]) << (8 * (B - I - 1))) | ...);
  } (std::make_index_sequence<B>());
}

template <unsigned B, std::size_t S>
constexpr auto read_le(std::span<const std::byte, S> data, std::size_t offset = 0) -> uint_t<B> {
  return read_le<uint_t<B>>(data, offset);
}

template <unsigned B, std::size_t S>
constexpr auto read_le(std::span<std::byte, S> data, std::size_t offset = 0) -> uint_t<B> {
  return read_le<uint_t<B>>(std::span<const std::byte, S> {data}, offset);
}

template <std::size_t S>
constexpr void write_le(std::span<std::byte, S> data, std::unsigned_integral auto uint,
                        std::size_t offset = 0) {
  static_assert(sizeof uint <= S);
  return [&]<auto... I>(std::index_sequence<I...>) {
    ((data[offset + I] = static_cast<std::byte>((uint >> (8 * I) & 0xff))), ...);
  } (std::make_index_sequence<sizeof uint>());
}

template <unsigned B, std::size_t S>
constexpr void write_le(std::span<std::byte, S> data, std::unsigned_integral auto uint,
                        std::size_t offset = 0) {
  return write_le(data, static_cast<uint_t<B>>(uint), offset);
}

template <std::unsigned_integral T, std::size_t S, std::size_t B = sizeof(T)>
constexpr auto read_be(std::span<const std::byte, S> data, std::size_t offset = 0) -> T {
  static_assert(B <= S);
  static_assert(B <= sizeof(T));

  return [&]<auto... I>(std::index_sequence<I...>) {
    return ((static_cast<T>(data[offset + I]) << (8 * I)) | ...);
  } (std::make_index_sequence<sizeof(T)>());
}

template <std::size_t S>
constexpr void write_be(std::span<std::byte, S> data, std::unsigned_integral auto uint,
                        std::size_t offset = 0) {
  constexpr auto B = sizeof uint;
  static_assert(B <= S);

  return [&]<auto... I>(std::index_sequence<I...>) {
    ((data[offset + I] = static_cast<std::byte>((uint >> (8 * (B - I - 1)) & 0xff))), ...);
  } (std::make_index_sequence<sizeof uint>());
}

} // cdb
