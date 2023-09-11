#pragma once

#include "core/error.hh"
#include "core/logger.hh"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>

namespace fs = std::filesystem;

namespace cdb::io {

class mm_file
{
private:
	using file_handle = int;

	std::byte *mem = nullptr;
	file_handle file = 0;
	std::size_t file_size = 0, mem_size = 0;

	// Windoze only: need to store another stupid file mapping handle.
	void *mh = nullptr;

public:
	mm_file() = default;
	~mm_file() { close(); }

	mm_file(const mm_file &) = delete;
	mm_file(mm_file &&f) noexcept
		: mem(std::exchange(f.mem, nullptr)),
		  file(std::exchange(f.file, 0)),
			file_size(std::exchange(f.file_size, 0)),
			mem_size(std::exchange(f.mem_size, 0))
			
	{
	}

	mm_file &operator=(const mm_file &) = delete;
	mm_file &operator=(mm_file &&f) noexcept
	{
		mem = std::exchange(f.mem, nullptr);
		file = std::exchange(f.file, 0);
		file_size = std::exchange(f.file_size, 0);
		mem_size = std::exchange(f.mem_size, 0);
		return *this;
	}

	std::error_code open(const fs::path &path, std::size_t size = 0, bool temp = false);
	void close();
	void sync();

	std::size_t size() const noexcept { return file_size; }
	std::size_t msize() const noexcept { return mem_size; }

	bool is_open() const noexcept { return file > 0; };

	std::span<std::byte> mutable_span() const noexcept { return {mem, file_size}; }
	std::span<const std::byte> span() const noexcept { return {mem, file_size}; }

	std::string_view str_view() const noexcept {
		return {reinterpret_cast<const char *>(mem), file_size};
	}
};

inline Result<mm_file> mm_open(const fs::path &path, std::size_t size = 0)
{
	if (mm_file f; auto ec = f.open(path, size))
		return std::unexpected(ec);
	else
		return f;
}

} // cdb::io
