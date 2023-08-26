
#include "core/error.hh"
#include "core/io.hh"
#include "core/logger.hh"

const cdb::log::logger logger {"io"};

using namespace cdb::io;

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

static const auto allocation_granularity = [] {
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);

  return static_cast<size_t>(systemInfo.dwAllocationGranularity);
}();
static const size_t page_mask = ~(allocation_granularity - 1);

static inline size_t round_up(size_t size) {
  // Assumes the page size is a multiple of two
  return (size + ~page_mask) & page_mask;
}

std::error_code mm_file::open(const fs::path &path, size_t size, bool temp)
{
  if (file > 0) // already open
    return IOError::AlreadyInUse;

  // use size of file
  if (size == 0) {
    std::error_code ec;
    size = fs::file_size(path, ec);

    if (ec) // could not get file size
      return ec;
  }

  file_size = size;
  mem_size = round_up(size);

  const std::string p = path.string();
  file = _open(p.c_str(), _O_CREAT | _O_RDWR, _S_IREAD | _S_IREAD);
  if (file < 0) // failed to open
    return {errno, std::generic_category()};

  if (temp) // mark file for deletion
    _unlink(p.c_str()); // once the process exits it will be deleted

  auto fh = (HANDLE)_get_osfhandle(file);
  auto mh = CreateFileMapping(fh, nullptr, PAGE_READWRITE,
                              mem_size >> 32, mem_size & 0xffffffff, nullptr);

  if (mh == nullptr) // CreateFileMapping failed
    return {static_cast<int>(GetLastError()), std::system_category()};

  auto desired_access = FILE_MAP_READ | FILE_MAP_WRITE;
  mem = reinterpret_cast<std::byte *>(MapViewOfFile(mh, desired_access, 0, 0, mem_size));
  if (mem == nullptr) { // MapViewOfFile failed
    CloseHandle(mh);
    return {static_cast<int>(GetLastError()), std::system_category()};
  }

  return {};
}

void mm_file::close() {
  if (!is_open())
    return;

  UnmapViewOfFile(mem);
  // SetFilePointer, SetEndOfFile
  // CloseHandle ???
  _close(file);

  file = -1;
  mem = nullptr;
  file_size = mem_size = 0;
}

#else

/* Memory mapped files following this tutorial:
 *  https://bertvandenbroucke.netlify.app/2019/12/08/memory-mapping-files/
 */
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static const size_t page_mask = ~(sysconf(_SC_PAGE_SIZE) - 1);

static inline size_t round_up(size_t size) {
  // Assumes the page size is a multiple of two
  return (size + ~page_mask) & page_mask;
}

std::error_code mm_file::open(const fs::path &path, size_t size, bool temp)
{
  if (file > 0) // already open
    return IOError::AlreadyInUse;

  // use size of file
  if (size == 0) {
    std::error_code ec;
    size = fs::file_size(path, ec);

    if (ec) // could not get file size
      return ec;
  }

  file_size = size;
  mem_size = round_up(size);

  file = ::open(path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (file < 0) // failed to open
    return {errno, std::generic_category()};

  if (temp) // mark file for deletion
    unlink(path.c_str()); // once the process exits it will be deleted

  if (posix_fallocate(file, 0, mem_size)) { // fallocate failed
    auto e = errno;
    ::close(file);
    file = -1;
    return {e, std::generic_category()};
  }

  mem = reinterpret_cast<std::byte *>(
    mmap(NULL, mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, file, 0));

  if (mem == MAP_FAILED) { // failed to memory map
    auto e = errno;
    ::close(file);
    file = -1;
    mem = nullptr;
    return {e, std::generic_category()};
  }

  return {};
}

void mm_file::close() {
  if (!is_open())
    return;

  if (munmap(mem, mem_size) < 0)
    logger.error("munmap({}, {}) failed\n", static_cast<const void *>(mem), mem_size);

  if (ftruncate(file, file_size) < 0)
    logger.error("ftruncate({}, {}) failed\n", file, file_size);

  if (::close(file) < 0)
    logger.error("close({}, {}) failed\n", file);

  file = -1;
  mem = nullptr;
  file_size = mem_size = 0;
}

#endif
