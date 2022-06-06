
#include <cstdint>
#include <iostream>
#include <string_view>
#include <type_traits>

#include "chess/util.h"

namespace io
{

class byte_writer
{
private:
	FILE *out;

public:
	byte_writer(FILE *out)
		: out(out)
	{
	}

	void write_bytes(const uint8_t *bytes, size_t n)
	{
		fwrite(bytes, sizeof(uint8_t), n, out);
	}

	void write_uint(uint64_t b, size_t n)
	{
		ASSERT(n <= sizeof b);
		fwrite(&b, n, 1, out);
	}

	void write_byte(uint8_t byte)
	{
		write_uint(byte, 1);
	}

	void write_uleb128(uint64_t value)
	{
		do
		{
			uint8_t byte = value & 0x7f;
			value >>= 7;

			write_byte(byte | ((value > 0) << 7));
		} while (value);
	}

	void write_string(std::string_view sv)
	{
		write_uleb128(sv.size());
		write_bytes((const uint8_t *)sv.data(), sv.size());
	}
};

////////////////////////////////////////////////////////////////////////////////

class byte_reader
{
private:
	std::istream &in;

public:
	byte_reader(std::istream &in)
		: in(in)
	{
	}

	bool eof() const { return in.eof(); }
	bool bad() const { return in.bad(); }

	void read_bytes(uint8_t *bytes, size_t n)
	{
		in.read((char *)bytes, n);
	}

	template <typename T = uint64_t>
	auto read_uint(size_t n)
	{
		static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>);
		ASSERT(n <= sizeof(T));

		T value = 0;
		in.read((char *)&value, n);

		return value;
	}

	auto read_byte()
	{
		return read_uint<uint8_t>(1);
	}

	template <typename T = uint64_t>
	auto read_uleb128()
	{
		T value = 0;

		for (int shift = 0; ; shift += 7)
		{
			T byte = read_byte();
			value |= (byte & 0x7f) << shift;

			if ((byte & 0x80) == 0)
				break;
		}

		return value;
	}

	std::string read_string()
	{
		uint64_t n = read_uleb128();
		
		std::string s;
		s.resize(n);

		read_bytes((uint8_t *)s.data(), n);
		return s;
	}
};

}
