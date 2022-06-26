
#include <string>
#include <string_view>

#include "io.hh"

namespace db
{

/**
 * 
 *  000-001 : \xBF
 *  001-008 : ChessDB
 *  008-009 : \x0A
 *  009-010 : \x1A
 *  010-012 : version number
 *  012-016 : length of data
 *  016-020 : offset to data
 *  020-024 : checksum
 *  024-032 : padding (nul)
 *  032-096 : name of database
 *  ...-... : padding up to 96 bytes if necessary
 */

constexpr std::string_view magic = "\u00bfChessDB\n\x1a";

template <typename T>
T read_le(std::string_view bytes)
{
	T x = 0;

	for (std::size_t i = 0; i < sizeof x; ++i)
		x |= static_cast<T>(bytes[i]) << ((sizeof x - i) * 8);

	return x;
}

struct Header
{
	Header() = default;
	Header(std::string_view data)
		: Header()
	{
		good = data.size() >= 96 && data.substr(0, 10) == magic;
		if (!good) return;

		version     = read_le<std::uint16_t>(data.substr(10));
		data_length = read_le<std::uint32_t>(data.substr(12));
		data_offset = read_le<std::uint32_t>(data.substr(16));
		checksum    = read_le<std::uint32_t>(data.substr(20));
		
		name = data.substr(32, 64);
	}

	bool good;

	std::uint16_t version;
	std::uint32_t data_length;
	std::uint32_t data_offset;
	std::uint32_t checksum;
	
	std::string name;
};

class Database
{
private:
	io::mm_file _file;
	Header _header;

public:
	Database()
	{
	}

	Database(const std::string &file_name);
	~Database();

	int open(const std::string &file_name);
	void close();
};

} // db
