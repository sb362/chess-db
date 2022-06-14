
#include "chess/util.h"

/* Memory mapped files following these tutorials:
 *  https://bertvandenbroucke.netlify.app/2019/12/08/memory-mapping-files/
 */
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

static size_t page_mask = 0;

// Memory-mapped file
struct MMFile
{
	char *mem;
	int file;
	size_t file_size, mem_size;
}

static inline size_t round_up(size_t size)
{
	ASSERT(page_mask);

	// Assumes the page size is a multiple of two
	return (size + ~page_mask) & page_mask;
}

struct MMFile *mmf_open(const char *file_name, size_t size = 0x1000)
{
	if (page_mask == 0)
		page_mask = ~(sysconf(_SC_PAGE_SIZE) - 1);

	struct MMFile *mmf = malloc(sizeof(struct MMFile));
	mmf->file_size = size;
	mmf->mem_size = round_up(size);

	mmf->file = open(file_name, O_CREAT | O_RDWR);
	
	if (mmf->file < 0)
	{
		fprintf(stderr, "Failed to open file '%s'\n", file_name);
		abort();
	}

	if (posix_fallocate(mmf->file, 0, mmf->mem_size) != 0)
	{
		fprintf(stderr, "%s : posix_fallocate() failed\n", file_name);
		abort();
	}

	mmf->mem = (char *)mmap(NULL, mmf->mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, mmf->file, 0);

	if (mmf->mem == MAP_FAILED)
	{
		fprintf(stderr, "%s : mmap() failed\n");
		abort();
	}

	return mmf;
}

void mmf_sync(struct MMFile *mmf)
{
	msync(mmf->mem, mmf->mem_size, MS_SYNC);
}

void mmf_close(struct MMFile *mmf)
{
	munmap(mmf->mem, mmf->mem_size);
	ftruncate(mmf->file, mmf->file_size);
	close(mmf->file);
	free(mmf);
}
