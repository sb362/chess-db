#pragma once

#include "util/source_location.hh"

#include <fmt/format.h>

#include <array>
#include <string>
#include <string_view>

namespace cdb::log
{

enum class log_level {
  trace, debug, info, warn, error, fatal
};

struct log_context {
  std::string_view fmt;
  source_location sloc;
  
  constexpr log_context(const char *fmt,
                        const source_location sloc = source_location::current())
      : fmt(fmt), sloc(sloc)
  {
  }

  constexpr log_context(std::string_view fmt,
                        const source_location sloc = source_location::current())
      : fmt(fmt), sloc(sloc)
  {
  }
};

class logger {
  std::string _name;
  log_level _level;

  void vlog(log_level level, std::string_view format, fmt::format_args args,
            const source_location sloc = source_location::current()) const;

public:
  constexpr logger(std::string name, log_level level = log_level::trace) : _name(std::move(name)), _level(level) {}

  constexpr bool enabled(log_level level) const { return level >= _level; }

  template <typename ...Args>
  void log(log_level level, log_context ctx, Args &&...args) const {
    if (enabled(level))
      vlog(level, ctx.fmt, fmt::make_format_args(std::forward<Args>(args)...), ctx.sloc);
  }

  template <typename ...Args>
  void trace(log_context ctx, Args &&...args) const { log(log_level::trace, ctx, std::forward<Args>(args)...); }

  template <typename ...Args>
  void debug(log_context ctx, Args &&...args) const { log(log_level::debug, ctx, std::forward<Args>(args)...); }

  template <typename ...Args>
  void info(log_context ctx, Args &&...args) const { log(log_level::info, ctx, std::forward<Args>(args)...); }

  template <typename ...Args>
  void warn(log_context ctx, Args &&...args) const { log(log_level::warn, ctx, std::forward<Args>(args)...); }

  template <typename ...Args>
  void error(log_context ctx, Args &&...args) const { log(log_level::error, ctx, std::forward<Args>(args)...); }
};

} // cdb::log

template <>
struct fmt::formatter<cdb::log::log_level> {
  template <class ParseContext>
  constexpr auto parse(ParseContext &pc) { return pc.begin(); }

  template <class FormatContext>
  auto format(cdb::log::log_level level, FormatContext &fc) const {
    static constexpr std::array level_strs {"TRACE", "DEBUG", "INFO ", "WARN ", "ERROR"};
    return fmt::format_to(fc.out(), "{}", level_strs[static_cast<int>(level)]);
  }
};
