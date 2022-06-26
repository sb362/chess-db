
#include "io.hh"

#include "fmt/format.h"

/* Memory mapped files following these tutorials:
 *  https://bertvandenbroucke.netlify.app/2019/12/08/memory-mapping-files/
 */
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace io;

static size_t page_mask = ~(sysconf(_SC_PAGE_SIZE) - 1);

static inline size_t round_up(size_t size)
{
	// Assumes the page size is a multiple of two
	return (size + ~page_mask) & page_mask;
}

int mm_file::open(const std::string &file_name, size_t size)
{
	if (file > 0)
		return -1; // already open

	file_size = size;
	mem_size = round_up(size);

	file = ::open(file_name.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (file < 0)
		return -2; // failed to open

	if (posix_fallocate(file, 0, mem_size) != 0)
	{
		::close(file);
		file = -1;
		return -3; // fallocate failed
	}

	mem = reinterpret_cast<char *>(
		mmap(NULL, mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, file, 0));

	if (mem == MAP_FAILED)
	{
		::close(file);
		file = -1;
		mem = nullptr;
		return -4; // failed to memory map
	}

	return 0;
}

void mm_file::close()
{
	if (!is_open())
		return;

	if (munmap(mem, mem_size) < 0)
		fmt::print(stderr, "munmap({}, {}) failed\n", mem, mem_size);

	if (ftruncate(file, file_size) < 0)
		fmt::print(stderr, "ftruncate({}, {}) failed\n", file, file_size);

	if (::close(file) < 0)
		fmt::print(stderr, "close({}, {}) failed\n", file);

	file = -1;
	mem = nullptr;
	file_size = mem_size = 0;
}

void mm_file::sync()
{
	msync(mem, mem_size, MS_SYNC);
}
