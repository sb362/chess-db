
#include "db.hh"

using namespace db;

Database::Database(const std::string &file_name)
	: _file(file_name, 0x1000), _header(_file.str())
{
		
}

Database::~Database()
{
	
}
