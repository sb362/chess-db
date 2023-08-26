#pragma once

#include "core/error.hh"

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

public:
	mm_file() = default;
	~mm_file() { close(); }

	mm_file(const mm_file &) = delete;
	mm_file(mm_file &&) = default;

	std::error_code open(const fs::path &path, std::size_t size = 0, bool temp = false);
	void close();
	void sync();

	std::size_t size() const { return file_size; }

	bool is_open() { return file > 0; };

	std::span<std::byte> mutable_span() { return {mem, file_size}; }
	std::span<const std::byte> span() const { return {mem, file_size}; }

	std::string_view str_view() const {
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
