#pragma once

#include "source_location.hh"
#include "logger.hh"

#include <array>
#include <chrono>
#include <format>
#include <ostream>
#include <string>

using namespace std::chrono_literals;

namespace cdb::log
{

using clock = std::chrono::high_resolution_clock;
static const auto t0 = clock::now();

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
  static std::ostream &os;

public:
  logger(std::string name) : _name(std::move(name)), _level(log_level::trace) {}

  bool enabled(log_level level) const { return level >= _level; }

  template <typename ...Args>
  void log(log_level level, log_context ctx, Args &&...args)
  {
    if (enabled(level))
    {
      const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - cdb::log::t0);
      const auto fmtted = std::vformat(ctx.fmt, std::make_format_args(std::forward<Args>(args)...));

      os << std::format("{} {:%T} {}:{} {} {}\n", 
                        level, dt, ctx.sloc.file_name(),
                        ctx.sloc.line(), ctx.sloc.function_name(), fmtted);
    }
  }

  template <typename ...Args>
  void trace(log_context ctx, Args &&...args) { log(log_level::trace, ctx, std::forward<Args>(args)...); }

  template <typename ...Args>
  void debug(log_context ctx, Args &&...args) { log(log_level::debug, ctx, std::forward<Args>(args)...); }

  template <typename ...Args>
  void info(log_context ctx, Args &&...args) { log(log_level::info, ctx, std::forward<Args>(args)...); }

  template <typename ...Args>
  void warn(log_context ctx, Args &&...args) { log(log_level::warn, ctx, std::forward<Args>(args)...); }

  template <typename ...Args>
  void error(log_context ctx, Args &&...args) { log(log_level::error, ctx, std::forward<Args>(args)...); }
};

} // log

template <>
struct std::formatter<cdb::log::log_level> {
  template <class ParseContext>
  constexpr auto parse(ParseContext &pc) { return pc.begin(); }

  template <class FormatContext>
  auto format(cdb::log::log_level level, FormatContext &fc) const {
    static constexpr std::array level_strs {"TRACE", "DEBUG", "INFO ", "WARN ", "ERROR"};
    return std::format_to(fc.out(), "{}", level_strs[static_cast<int>(level)]);
  }
};
