
#include <string>

/* Memory mapped files following these tutorials:
 *  https://bertvandenbroucke.netlify.app/2019/12/08/memory-mapping-files/
 */
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

static size_t page_mask = 0;

class MemoryMappedFile
{
private:
	char *mem;
	int file;
	size_t file_size, mem_size;

public:
	MemoryMappedFile(const std::string &file_name, size_t size)
		: mem(nullptr), file(0), file_size(0), mem_size(round_up(size))
	{
		
	}

	~MemoryMappedFile()
	{
		close();
	}
};

static inline size_t round_up(size_t size)
{
	// Assumes the page size is a multiple of two
	return (size + ~page_mask) & page_mask;
}

struct MappedFile *mf_open(const char *file_name, size_t size = 0x1000)
{
	if (page_mask == 0)
		page_mask = ~(sysconf(_SC_PAGE_SIZE) - 1);

	struct MappedFile *mf = malloc(sizeof(struct MappedFile));
	mf->file_size = size;
	mf->mem_size = round_up(size);

	mf->file = open(file_name.c_str(), O_CREAT | O_RDWR);
	
	if (mf->file < 0)
	{
		fprintf(stderr, "Failed to open file '%s'\n", file_name);
		abort();
	}

	if (posix_fallocate(db.file, 0, db.mem_size) != 0)
	{
		fmt::print(stderr, "{} : posix_fallocate() failed\n", file_name);
		abort();
	}

	db.mem = reinterpret_cast<char *>(
		mmap(NULL, db.mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, db.file, 0));

	if (db.mem == MAP_FAILED)
	{
		fmt::print("{} : mmap() failed\n");
		abort();
	}
}

void db_sync(Database db)
{
	msync(db.mem, db.mem_size, MS_SYNC);
}

void db_close(Database db)
{
	munmap(db.mem, db.mem_size);
	db.mem = NULL;

	ftruncate(db.file, db.file_size);

	close(db.file);
	db.file = 0;
}
