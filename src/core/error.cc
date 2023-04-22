
#include "core/error.hh"

#include <algorithm> // std::clamp

struct CoreCategory : std::error_category {
  [[nodiscard]] const char *name() const noexcept override { return "core"; }
  [[nodiscard]] std::string message(int ev) const override {
    using enum CoreError;
    switch (static_cast<CoreError>(ev)) {
    case NotImplemented:
      return "not implemented";
    default:
      return "(unknown error)";
    }
  }
};

std::error_code make_error_code(CoreError e) {
  static CoreCategory category;
  return {static_cast<int>(e), category};
}

struct IOCategory : std::error_category {
  [[nodiscard]] const char *name() const noexcept override { return "io"; }
  [[nodiscard]] std::string message(int ev) const override {
    using enum IOError;
    switch (static_cast<IOError>(ev)) {
    case FileNotFound:     return "file not found";
    case FileExists:       return "file exists";
    case PermissionDenied: return "permission denied";
    case NotEnoughSpace:   return "not enough space";
    case Timeout:          return "timed out";
    case AlreadyInUse:     return "already in use";
    default:               return "(unknown error)";
    }
  }
};

std::error_code make_error_code(IOError e) {
  static IOCategory category;
  return {static_cast<int>(e), category};
}

struct ParseCategory : std::error_category {
  [[nodiscard]] const char *name() const noexcept override { return "parse"; }
  [[nodiscard]] std::string message(int ev) const override {
    using enum ParseError;
    switch (static_cast<ParseError>(ev)) {
    case Invalid:   return "invalid input";
    case Illegal:   return "illegal input";
    case Ambiguous: return "ambiguous input";
    case Reserved:  return "reserved token";
    default:        return "(unknown error)";
    }
  }
};

std::error_code make_error_code(ParseError e) {
  static ParseCategory category;
  return {static_cast<int>(e), category};
}

struct DbCategory : std::error_category {
  [[nodiscard]] const char *name() const noexcept override { return "db"; }
  [[nodiscard]] std::string message(int ev) const override {
    using enum DbError;
    switch (static_cast<DbError>(ev)) {
    case BadMagic:    return "bad magic";
    case BadChecksum: return "bad checksum";
    default:          return "(unknown error)";
    }
  }
};

std::error_code make_error_code(DbError e) {
  static DbCategory category;
  return {static_cast<int>(e), category};
}


std::string_view cdb::get_context(std::string_view s, std::size_t pos, std::size_t max_size) {
  auto l = std::clamp(std::size_t(pos - max_size / 2), std::size_t(0), s.size());
  auto r = std::clamp(            pos + max_size / 2,  pos,  s.size());

  return s.substr(l, r - pos);
}
