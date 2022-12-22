#pragma once

#include <string_view>

namespace cdb {

class source_location {
  std::string_view _func, _file;
  unsigned _line, _col;

public:
  constexpr source_location(std::string_view func = "<unknown>",
                            std::string_view file = "<unknown>",
                            unsigned line = 0,
                            unsigned col = 0)
   : _func(func), _file(file), _line(line), _col(col)
  {
  }

  static constexpr source_location current(std::string_view func = __builtin_FUNCTION(),
                                           std::string_view file = __builtin_FILE(),
                                           unsigned line = __builtin_LINE(),
                                           unsigned col = 0)
  {
    return {func, file, line, col};
  }

  constexpr std::string_view function_name() const { return _func; }
  constexpr std::string_view file_name() const { return _file; }
  constexpr unsigned line() const { return _line; }
  constexpr unsigned column() const { return _col; }
};

} // cdb

