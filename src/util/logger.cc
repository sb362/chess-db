
#include "logger.hh"

#include <chrono>
#include <iostream>

using namespace cdb::log;

using hr_clock = std::chrono::high_resolution_clock;
static const auto t0 = hr_clock::now();

void logger::vlog(log_level level, std::string_view fmt, std::format_args args,
                  const source_location sloc)
{
    const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(hr_clock::now() - t0);
    std::cerr << std::format("{} {:%T} {}:{} {} {}\n", 
                             level, dt, sloc.file_name(),
                             sloc.line(), sloc.function_name(),
                             std::vformat(fmt, args));
}
