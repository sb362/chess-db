
#include "core/logger.hh"
#include "db/database.hh"

#include <algorithm>
#include <iostream>
#include <string_view>
#include <vector>

using namespace cdb;

int main(int argc, char *argv[]) {
  const std::vector<std::string_view> args(argv + 1, argv + argc);

  auto option_exists = [&args] (std::string_view name) {
    return std::ranges::contains(args, name);
  };

  auto option_value = [&args] (std::string_view name) {
    auto it = std::ranges::find(args, name);
    return (it != args.end() && (it + 1) != args.end()) ? *(it + 1) : "";
  };

  if (option_exists("-v"))
    log().set_level(log_level::trace);

  auto db = db::Database::open("test.cdb", {
    .create = true,
    .in_memory = false,
    .size = 4096,
  });

  if (!db) std::cout << db.error().message() << '\n';
  assert(db);

  db->flush();
  return 0;
}
