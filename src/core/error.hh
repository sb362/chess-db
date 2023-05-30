#pragma once

#include "util/misc.hh"

#include <charconv>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

template <class T, class E = std::error_code>
using Result = std::expected<T, E>;

enum class CoreError {
  Success = 0,
  NotImplemented,
  OutOfRange
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


enum class FENParseError {
  Success = 0,
  UnexpectedInPiecePlacement,
  IncompletePiecePlacement,
  InvalidSideToMove,
  InvalidCastling,
  InvalidEPSquare,
  MissingSpace
};

template <> struct std::is_error_code_enum<FENParseError> : std::true_type {};
std::error_code make_error_code(FENParseError e);

enum class SANParseError {
  Success = 0,
  InvalidInput,
  InvalidFile,
  InvalidRank,
  InvalidPiece,
  Ambiguous,
  MissingPiece,
};

template <> struct std::is_error_code_enum<SANParseError> : std::true_type {};
std::error_code make_error_code(SANParseError e);

enum class PGNParseError {
  Success = 0,
  UnterminatedQuote,
  UnterminatedTag,
  UnterminatedComment,
  UnterminatedVariation,
  MalformedResultToken,
  InvalidMoveNumber,
  ReservedToken,
  MalformedTag,
  NotInVariation,

  UnsupportedVariant,
  CustomFENNotImplemented,
};
template <> struct std::is_error_code_enum<PGNParseError> : std::true_type {};
std::error_code make_error_code(PGNParseError e);


namespace cdb {
  struct ParseResult {
    std::size_t pos;
    std::error_code ec = PGNParseError::Success;
    std::string_view context = "";

    explicit operator bool() const noexcept {
      return !ec;
    }
  };

  std::string_view get_context(std::string_view s, std::size_t pos, std::size_t max_size);

  template <Numeric T>
  Result<T> parse_numerical(std::string_view s) {
    T x;
    std::errc err = std::from_chars(s.data(), s.data() + s.size(), x).ec;
    if (err == std::errc()) {
      return std::unexpected(err);
    } else {
      return x;
    }
  };
} // cdb
