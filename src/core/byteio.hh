#pragma once

#include "util/bits.hh"
#include "util/komihash.hh"

#include <cstdint>
#include <memory>
#include <string_view>
#include <type_traits>

namespace cdb::io {

template <class T, class This>
class basic_buffer
{
public:
  using value_type = T;
  using pointer_type = value_type *;
  using size_type = std::size_t;

private:
  std::unique_ptr<value_type []> _ptr = nullptr;
  pointer_type _data = nullptr;
  size_type _size = 0, _pos = 0;

public:
  constexpr basic_buffer() = default;

  constexpr basic_buffer(pointer_type data, size_type size) noexcept
    : _data(data), _size(size)
  {
  }

  template <std::size_t S>
  constexpr basic_buffer(std::span<value_type, S> data) noexcept
    : _data(data.data()), _size(data.size())
  {
  }

  constexpr basic_buffer(size_type size)
    : _ptr(std::make_unique<value_type []>(size)), _data(_ptr.get()), _size(size)
  {
  }

  constexpr basic_buffer(basic_buffer &&b) noexcept
    : _ptr(std::move(b._ptr)), _data(b._data), _size(b._size), _pos(b._pos)
  {
  }

  constexpr basic_buffer &operator=(basic_buffer &&b) noexcept
  {
    _ptr = std::move(b._ptr);
    _data = b._data;
    _size = b._size;
    _pos = b._pos;
  }

  constexpr bool is_view() const noexcept {
    return _ptr == nullptr;
  }

  constexpr void resize(size_type size) noexcept {
    assert(!is_view());
    _ptr = std::make_unique<value_type []>(size);
    _data = _ptr.get();
    _size = size;
  }

  constexpr pointer_type data()  const noexcept { return _data; }
  constexpr size_type size()     const noexcept { return _size; }

  constexpr pointer_type begin() const noexcept { return _data; }
  constexpr pointer_type end()   const noexcept { return _data + size(); }

  constexpr value_type &at(size_type p) noexcept { return _data[p]; }
  constexpr value_type const &at(size_type p) const noexcept { return _data[p]; }
  constexpr value_type &operator[](size_type p) noexcept { return at(p); }
  constexpr value_type const &operator[](size_type p) const noexcept { return at(p); }

  constexpr bool empty() const noexcept { return size() == 0; }

  constexpr This subbuf(size_type offset, size_type sub_size) const noexcept {
    return {data() + offset, sub_size};
  }

  constexpr size_type pos() const noexcept { return _pos; }

  constexpr void seek(size_type n) noexcept { _pos += n; }
  constexpr void seek_abs(size_type n) noexcept { _pos = n; }

  std::uint64_t hash(std::uint64_t seed = 0) const noexcept { return komihash(data(), size(), seed); }

  constexpr std::byte peek() const noexcept { return static_cast<std::byte>(at(pos())); }

  template <std::unsigned_integral Ret>
  constexpr Ret read_le(size_type n = sizeof(Ret)) noexcept {
    Ret x = 0;

    for (size_t i = 0; i < n; ++i)
      x |= static_cast<Ret>(at(pos() + i)) << (8 * (n - i - 1));

    seek(n);
    return x;
  }

  template <std::size_t S>
  constexpr auto read_le() noexcept {
    return read_le<uint_t<S>>(S);
  }

  constexpr std::byte read_byte() noexcept {
    return static_cast<std::byte>(read_le<std::uint8_t>());
  }

  template <std::unsigned_integral Ret = std::uint64_t>
  constexpr Ret read_uleb128() noexcept {
    Ret value = 0;

    for (int shift = 0; ; shift += 7) {
      Ret byte = read_le<1>();
      value |= (byte & 0x7f) << shift;

      if ((byte & 0x80) == 0)
        break;
    }

    return value;
  }

  constexpr std::span<value_type> read_bytes(size_type n) noexcept {
    seek(n);
    return {data() + pos() - n, n};
  }

  constexpr std::string_view read_string() noexcept {
    return read_bytes(read_uleb128());
  }

  constexpr std::span<const std::byte> span_upto() const noexcept {
    return {data(), pos()};
  }
};

struct const_buffer : public basic_buffer<const std::byte, const_buffer>
{
  using basic_buffer::basic_buffer;

  constexpr const_buffer view() const noexcept {
    return {data(), size()};
  }
};

struct mutable_buffer : public basic_buffer<std::byte, mutable_buffer>
{
  using basic_buffer::basic_buffer;

  template <std::unsigned_integral T>
  constexpr void write_le(T x, size_type n = sizeof(T)) noexcept {
    for (size_t i = 0; i < n; x >>= 8, ++i)
      at(pos() + i) = static_cast<std::byte>(x & 0xff);

    seek(n);
  }

  constexpr void write_byte(value_type b) noexcept {
    write_le(static_cast<std::uint8_t>(b));
  }

  template <std::size_t S>
  constexpr void write_bytes(std::span<std::add_const_t<value_type>, S> bytes) noexcept {
    for (size_type i = 0; i < bytes.size(); ++i)
      write_byte(bytes[i]);
  }

  constexpr void write_uleb128(std::unsigned_integral auto value) noexcept {
    do {
      std::uint8_t byte = value & 0x7f;
      value >>= 7;

      write_le<std::uint8_t>(byte | ((value > 0) << 7));
    } while (value);
  }

  constexpr void write_string(std::string_view str) noexcept {
    write_uleb128(str.size());
    write_bytes(std::as_bytes(std::span {str.data(), str.size()}));
  }

  constexpr const_buffer view() const noexcept {
    return {data(), size()};
  }
};

} // cdb::io
