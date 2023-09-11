
#include "core/logger.hh"

#include <fmt/chrono.h>

#include <cassert>
#include <chrono>

using namespace cdb;

using hr_clock = std::chrono::high_resolution_clock;
static const auto t0 = hr_clock::now();

void Logger::vlog(log_level level, std::string_view format, fmt::format_args args,
                  const source_location sloc) const
{
  const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(hr_clock::now() - t0);
  fmt::print(stderr, "{} {:%T} {}:{} {} {}\n", 
                     level, dt, sloc.file_name(),
                     sloc.line(), sloc.function_name(),
                     fmt::vformat(format, args));
}

static Logger logger;
Logger &cdb::log() { return logger; }
