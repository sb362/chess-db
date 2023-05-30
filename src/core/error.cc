
#include "core/error.hh"

#include <algorithm> // std::clamp

struct CoreCategory : std::error_category {
  [[nodiscard]] const char *name() const noexcept override { return "core"; }
  [[nodiscard]] std::string message(int ev) const override {
    using enum CoreError;
    switch (static_cast<CoreError>(ev)) {
    case Success:
      return "success";
    case NotImplemented:
      return "not implemented";
    case OutOfRange:
      return "out of range";
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
    case Success:          return "success";
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

struct FENParseCategory : std::error_category {
  [[nodiscard]] const char *name() const noexcept override { return "fen"; }
  [[nodiscard]] std::string message(int ev) const override {
    using enum FENParseError;
    switch (static_cast<FENParseError>(ev)) {
    case Success:                    return "success";
    case UnexpectedInPiecePlacement: return "unexpected char in piece placement";
    case IncompletePiecePlacement:   return "incomplete piece placement";
    case InvalidSideToMove:          return "invalid side to move";
    case InvalidCastling:            return "invalid castling rights";
    case InvalidEPSquare:            return "invalid en passant square";
    case MissingSpace:               return "missing space";
    default:                         return "(unknown error)";
    }
  }
};

std::error_code make_error_code(FENParseError e) {
  static FENParseCategory category;
  return {static_cast<int>(e), category};
}

struct SANParseCategory : std::error_category {
  [[nodiscard]] const char *name() const noexcept override { return "san"; }
  [[nodiscard]] std::string message(int ev) const override {
    using enum SANParseError;
    switch (static_cast<SANParseError>(ev)) {
    case Success:       return "success";
    case InvalidInput:  return "invalid input";
    case InvalidFile:   return "invalid file";
    case InvalidRank:   return "invalid rank";
    case InvalidPiece:  return "invalid piece";
    case Ambiguous:     return "ambiguous move";
    case MissingPiece:  return "no piece to move";
    default:            return "(unknown error)";
    }
  }
};

std::error_code make_error_code(SANParseError e) {
  static SANParseCategory category;
  return {static_cast<int>(e), category};
}

struct PGNParseCategory : std::error_category {
  [[nodiscard]] const char *name() const noexcept override { return "pgn"; }
  [[nodiscard]] std::string message(int ev) const override {
    using enum PGNParseError;
    switch (static_cast<PGNParseError>(ev)) {
    case Success:               return "success";
    case UnterminatedTag:       return "unterminated tag";
    case UnterminatedQuote:     return "unterminated quote";
    case UnterminatedComment:   return "unterminated comment";
    case UnterminatedVariation: return "unterminated variation";
    case MalformedResultToken:  return "malformed result token";
    case InvalidMoveNumber:     return "invalid move number";
    case ReservedToken:         return "reserved token";
    case MalformedTag:          return "malformed tag";
    case NotInVariation:        return "not in variation";
    default:                    return "(unknown error)";
    }
  }
};

std::error_code make_error_code(PGNParseError e) {
  static PGNParseCategory category;
  return {static_cast<int>(e), category};
}


std::string_view cdb::get_context(std::string_view s, std::size_t pos, std::size_t max_size) {
  auto l = std::clamp(std::size_t(pos - max_size / 2), std::size_t(0), s.size());
  auto r = std::clamp(            pos + max_size / 2,  pos,  s.size());

  return s.substr(l, r - pos);
}
