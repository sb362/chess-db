#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

template <class T, class E = std::error_code>
using Result = std::expected<T, E>;

enum class CoreError {
  Success = 0,
  NotImplemented
};

template <> struct std::is_error_code_enum<CoreError> : std::true_type {};
std::error_code make_error_code(CoreError e);

enum class IOError {
  Success = 0,
  FileNotFound,
  FileExists,
  PermissionDenied,
  NotEnoughSpace,
  Timeout,
  AlreadyInUse
};

template <> struct std::is_error_code_enum<IOError> : std::true_type {};
std::error_code make_error_code(IOError e);

enum class ParseError {
  Success = 0,
  Invalid,
  Illegal,
  Ambiguous,
  Reserved
};

template <> struct std::is_error_code_enum<ParseError> : std::true_type {};
std::error_code make_error_code(ParseError e);

enum class DbError {
  Success = 0,

  BadMagic,
  BadChecksum,

  OutOfMemory,
};

template <> struct std::is_error_code_enum<DbError> : std::true_type {};
std::error_code make_error_code(DbError e);

namespace cdb {
  std::string_view get_context(std::string_view s, std::size_t pos, std::size_t max_size);
} // cdb
