#pragma once

#include "chess/util.h"

#include <string>
#include <cstring>

#include "fmt/format.h" // todo: removeme

namespace io
{

class mm_file
{
private:
	char *mem;
	int file;
	size_t file_size, mem_size;

public:
	mm_file()
		: mem(nullptr), file(0), file_size(0), mem_size(0)
	{
	}

	mm_file(const std::string &file_name, size_t size)
		: mm_file()
	{
		open(file_name, size);
	}

	~mm_file()
	{
		close();
	}

	int open(const std::string &file_name, size_t size = 0x1000);
	void close();
	void sync();

	bool is_open() { return file > 0; };

	const char *data() const { return mem; }
	char *data() { return mem; }

	std::string_view str() { return {mem, file_size}; }
	const std::string_view str() const { return {mem, file_size}; }
};

////////////////////////////////////////////////////////////////////////////////

struct buffer_t
{
	char *data = nullptr;
	size_t pos = 0;

	uint64_t bits = 0;
	size_t bit_pos = 0;
};

inline void write_bytes(buffer_t &out, const void *bytes, size_t n)
{
	memcpy(out.data + out.pos, bytes, n);
	out.pos += n;
}

inline void write_bytes(buffer_t &out, std::string_view bytes)
{
	write_bytes(out, bytes.data(), bytes.length());
}

inline void write_uint(buffer_t &out, uint64_t b, size_t n)
{
	ASSERT(n <= sizeof b);
	write_bytes(out, &b, n);
}

inline void write_byte(buffer_t &out, uint8_t byte)
{
	write_uint(out, byte, 1);
}

inline void write_uleb128(buffer_t &out, uint64_t value)
{
	do
	{
		uint8_t byte = value & 0x7f;
		value >>= 7;

		write_byte(out, byte | ((value > 0) << 7));
	} while (value);
}

inline void write_string(buffer_t &out, std::string_view sv)
{
	write_uleb128(out, sv.size());
	write_bytes(out, sv.data(), sv.size());
}

////////////////////////////////////////////////////////////////////////////////

inline std::string_view read_bytes(buffer_t &in, size_t n)
{
	in.pos += n;
	return {in.data + in.pos - n, n};
}

template <typename T = uint64_t>
inline auto read_uint(buffer_t &in, size_t n)
{
	static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>);
	ASSERT(n <= sizeof(T));

	T value = 0;
	memcpy(&value, in.data + in.pos, n);
	in.pos += n;
	return value;
}

inline uint8_t read_byte(buffer_t &in)
{
	uint8_t byte = *(in.data + in.pos);
	++in.pos;
	return byte;
}

template <typename T = uint64_t>
inline auto read_uleb128(buffer_t &in) -> T
{
	T value = 0;

	for (int shift = 0; ; shift += 7)
	{
		T byte = read_byte(in);
		value |= (byte & 0x7f) << shift;

		if ((byte & 0x80) == 0)
			break;
	}

	return value;
}

inline std::string_view read_string(buffer_t &in)
{
	return read_bytes(in, read_uleb128(in));
}

inline void flush_bits(buffer_t &out)
{
	if (out.bit_pos > 0)
	{
		write_uint(out, out.bits, (out.bit_pos + 7) / 8);
		out.bits = 0;
		out.bit_pos = 0;
	}
}

inline void write_bits(buffer_t &out, uint64_t bits, size_t n_bits)
{
	ASSERT(n_bits <= 64);
	ASSERT(msb(bits) < n_bits);

	size_t re = 64 - out.bit_pos;
	out.bits |= bits << out.bit_pos;
	out.bit_pos += n_bits;
	out.bit_pos %= 64;

	if (n_bits == re)
	{
		write_uint(out, out.bits, 8);
		out.bits = 0;
	}
	else if (n_bits > re)
	{
		write_uint(out, out.bits, 8);
		out.bits = bits >> (64 - out.bit_pos);
	}
}

inline void reset_bits(buffer_t &in)
{
	in.pos -= (64 - in.bit_pos) / 8;
	in.bits = 0;
	in.bit_pos = 0;
}

inline uint64_t read_bits(buffer_t &in, size_t n_bits)
{
	ASSERT(n_bits <= 64);

	if (in.bit_pos == 0)
		in.bits = read_uint(in, 8);

	size_t re = 64 - in.bit_pos;
	
	uint64_t bits = in.bits & ((1ull << n_bits) - 1);
	in.bits >>= n_bits;
	in.bit_pos += n_bits;
	in.bit_pos %= 64;

	if (n_bits > re)
	{
		in.bits = read_uint(in, 8);
		bits |= in.bits & ((1ull << in.bit_pos) - 1);
	}

	return bits;
}

} // io
