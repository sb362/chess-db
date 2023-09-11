
#include "core/byteio.hh"

#include <doctest.h>
#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <fstream> // todo: remove
#include <span>
#include <string>

using namespace cdb;

TEST_SUITE("byteio") {
  TEST_CASE("basic") {
    std::string_view str = "123456789";
    std::span s {str.data(), str.size()};
    auto bytes = std::as_bytes(s);

    io::const_buffer buf(bytes);
    
    CHECK(buf.is_view());
    CHECK(buf.size() == str.size());
    CHECK(buf.data() == bytes.data());
    CHECK(buf.peek() == std::byte(s[0]));
    CHECK(buf.read_byte() == std::byte(s[0]));
    CHECK(buf.pos() == 1);
    CHECK(buf.peek() == std::byte(s[1]));
    CHECK(buf.read_byte() == std::byte(s[1]));
    
    buf.seek_abs(0);
    CHECK(buf.peek() == std::byte(s[0]));
    CHECK(buf.read_byte() == std::byte(s[0]));

    CHECK(std::ranges::equal(buf.str_view(), str));
  }

  TEST_CASE("read_bytes") {
    std::string_view str = "\1\2\3abc\n;\4\0\5\6\789";
    std::span s {str.data(), str.size()};
    auto bytes = std::as_bytes(s);

    io::const_buffer buf(bytes);
    CHECK(buf.size() == bytes.size());
    CHECK(std::ranges::equal(buf.read_bytes(bytes.size()), bytes));
  }

  TEST_CASE("read_le") {
    std::array<uint8_t, 4> u32b {{0x12, 0xcd, 0x3e, 0x00}};
    auto bytes = std::as_bytes(std::span {u32b.data(), u32b.size()});

    io::const_buffer buf(bytes);
    auto u32 = buf.read_le<4>();

    auto expected = uint32_t(u32b[0])
                  | uint32_t(u32b[1]) << 8
                  | uint32_t(u32b[2]) << 16
                  | uint32_t(u32b[3]) << 24;
    CHECK(u32 == expected);

    CHECK(buf.remaining() == 0);
    buf.seek_abs(0);
    CHECK(buf.read_le<uint32_t>() == u32);
    buf.seek_abs(0);

    expected = u32b[0] | u32b[1] << 8;
    CHECK(buf.read_le<2>() == expected);

    expected = u32b[2] | u32b[3] << 8;
    CHECK(buf.read_le<2>() == expected);
  }

  TEST_CASE("read/write_string") {
    constexpr std::string_view s1 = "[];";
    constexpr std::string_view s2 = "abc\nxyz\0 123\r";
    io::mutable_buffer out_buf(32);
    out_buf.write_string(s1);
    out_buf.write_string(s2);

    io::const_buffer in_buf(out_buf.view());

    CHECK(in_buf.is_view());
    CHECK(in_buf.size() == out_buf.size());
    CHECK(in_buf.data() == out_buf.data());

    CHECK(std::ranges::equal(in_buf.read_string(), s1));
    CHECK(std::ranges::equal(in_buf.read_string(), s2));
    CHECK(in_buf.pos() == out_buf.pos());

    auto rem = out_buf.size() - out_buf.pos();
    CHECK(in_buf.remaining() == rem);
  }

  TEST_CASE("read/write_le") {
    constexpr std::uint64_t u0 = 158;
    constexpr std::uint64_t u1 = 35293;
    constexpr std::uint64_t u2 = 0;
    constexpr std::uint64_t u3 = 12395672695;
    constexpr std::uint64_t u4 = 32;
    constexpr std::uint64_t u5 = 3333;

    io::mutable_buffer out_buf(32);
    out_buf.write_le(u0, 1);
    out_buf.write_le(u0, 2);
    out_buf.write_le(u1, 2);
    out_buf.write_le(u2, 4);
    out_buf.write_le(u3, 8);
    out_buf.write_le(u4, 5);
    out_buf.write_le(u5, 3);

    io::const_buffer in_buf(out_buf.view());

    CHECK(in_buf.read_le<1>() == u0);
    CHECK(in_buf.read_le<2>() == u0);
    CHECK(in_buf.read_le<2>() == u1);
    CHECK(in_buf.read_le<4>() == u2);
    CHECK(in_buf.read_le<8>() == u3);
    CHECK(in_buf.read_le<5>() == u4);
    CHECK(in_buf.read_le<3>() == u5);
    CHECK(in_buf.pos() == out_buf.pos());
  }

  TEST_CASE("read/write_uleb128") {
    constexpr std::uint64_t u0 = 0;
    constexpr std::uint64_t u1 = 293578239;
    constexpr std::uint64_t u2 = 1;
    constexpr std::uint64_t u3 = 937523758157125682;
    constexpr std::uint64_t u4 = -1;

    io::mutable_buffer out_buf(64);
    out_buf.write_uleb128(u0);
    out_buf.write_uleb128(u1);
    out_buf.write_uleb128(u2);
    out_buf.write_uleb128(u3);
    out_buf.write_uleb128(u4);

    io::const_buffer in_buf(out_buf.view());
    CHECK(in_buf.read_uleb128() == u0);
    CHECK(in_buf.read_uleb128() == u1);
    CHECK(in_buf.read_uleb128() == u2);
    CHECK(in_buf.read_uleb128() == u3);
    CHECK(in_buf.read_uleb128() == u4);
  }

  TEST_CASE("subbuf") {
    constexpr std::uint64_t u0 = 0x2a37b9cd5u;
    constexpr std::uint64_t u1 = 0x456abcu;
    constexpr std::uint64_t u2 = 0x9abu;

    io::mutable_buffer out_buf(64);
    out_buf.write_le(u0, 6);
    out_buf.write_le(u1, 6);
    out_buf.write_le(u2, 3);

    int offset = 6, size = 9;
    auto ss = out_buf.subbuf(offset, size);
    CHECK(ss.data() == out_buf.data() + offset);
    CHECK(ss.size() == size);
    CHECK(ss.pos() == 0);
    CHECK(ss.is_view());

    CHECK(ss.read_le<std::uint64_t>(6) == u1);
    CHECK(ss.read_le<std::uint64_t>(3) == u2);
  }
}
