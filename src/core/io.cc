
#include "core/error.hh"
#include "core/io.hh"
#include "core/logger.hh"

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

	log().debug("io: mapping {}", path);
  std::error_code ec;

  // use size of file
  if (size == 0) {
    size = fs::file_size(path, ec);

    if (ec) // could not get file size
      return ec;
  }

  file_size = size;
  mem_size = round_up(size);

  auto open_perms = _O_RDWR;
  if (!fs::exists(path))
    open_perms |= _O_CREAT;

  const std::string p = path.string();
  file = _open(p.c_str(), open_perms, _S_IREAD | _S_IWRITE);
  if (file < 0) // failed to open
    return {errno, std::generic_category()};

  if (temp) // mark file for deletion
    _unlink(p.c_str()); // once the process exits it will be deleted

  auto fh = reinterpret_cast<HANDLE>(_get_osfhandle(file));
  mh = CreateFileMapping(fh, nullptr, PAGE_READWRITE,
                         mem_size >> 32, mem_size & 0xffffffff, nullptr);

  // todo: refactor this garbage
  if (mh == nullptr) { // CreateFileMapping failed
    ec = {static_cast<int>(GetLastError()), std::system_category()};
    log().error("io: CreateFileMapping failed - {} ()", ec.message(), ec.value());

    _close(file);
    file = 0;
    return ec;
  }

  auto desired_access = FILE_MAP_READ | FILE_MAP_WRITE;
  mem = static_cast<std::byte *>(MapViewOfFile(mh, desired_access, 0, 0, mem_size));
  if (mem == nullptr) { // MapViewOfFile failed
    ec = {static_cast<int>(GetLastError()), std::system_category()};
    log().error("io: MapViewOfFile failed - {} ()", ec.message(), ec.value());

    CloseHandle(mh);
    _close(file);
    file = 0;
    return ec;
  }

  log().info("io: mapped {} (fh = {}, msize = {}, fsize = {})", path, file, mem_size, file_size);
  return {};
}

void mm_file::close() {
  if (!is_open())
    return;

  std::error_code ec;
  log().info("io: unmapping file fh = {}, mem = {}",
             file, static_cast<const void *>(mem));

  if (!UnmapViewOfFile(mem)) {
    ec = {static_cast<int>(GetLastError()), std::system_category()};
    log().error("io: UnmapViewOfFile failed - {} ({})", ec.message(), GetLastError());
  }

  CloseHandle(mh);

  auto fh = reinterpret_cast<HANDLE>(_get_osfhandle(file));
  long low = file_size & 0xffffffff, high = file_size >> 32;
  if (SetFilePointer(fh, low, &high, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
    ec = {static_cast<int>(GetLastError()), std::system_category()};
    log().error("io: SetFilePointer failed - {} ({})", ec.message(), GetLastError());
  } else {
    if (!SetEndOfFile(fh)) {
      ec = {static_cast<int>(GetLastError()), std::system_category()};
      log().error("io: SetEndOfFile failed - {} ({})", ec.message(), GetLastError());
    }
  }

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
    log().error("io: munmap({}, {}) failed\n", static_cast<const void *>(mem), mem_size);

  if (ftruncate(file, file_size) < 0)
    log().error("io: ftruncate({}, {}) failed\n", file, file_size);

  if (::close(file) < 0)
    log().error("io: close({}, {}) failed\n", file);

  file = -1;
  mem = nullptr;
  file_size = mem_size = 0;
}

#endif
