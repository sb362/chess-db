#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <tuple>
#include <type_traits>

namespace cdb {
  // https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/
  template <std::size_t N>
  struct Literal {
    char str[N] = {};

    constexpr Literal(const char (&s)[N]) {
      std::copy_n(s, N, str);
    }

    [[nodiscard]] constexpr std::size_t size() const {
      return sizeof str;
    }

    [[nodiscard]] constexpr const char *data() const {
      return str;
    }
  };

  struct bytes_base {
    std::size_t n;

    constexpr bytes_base(std::size_t n_bytes = 0) : n(n_bytes) {}
  };

  template <std::size_t Factor, Literal Unit>
  struct bytes_ : bytes_base {
    static constexpr auto factor = Factor;
    static constexpr auto units = Unit.data();
  };

  struct bytes     : bytes_<1ull << 0, "B"> {};
  struct kibibytes : bytes_<1ull << 10, "KiB"> {};
  struct mebibytes : bytes_<1ull << 20, "MiB"> {};
  struct gibibytes : bytes_<1ull << 30, "GiB"> {};
  struct tebibytes : bytes_<1ull << 40, "TiB"> {};

  struct kilobytes : bytes_<1'000,             "KB"> {};
  struct megabytes : bytes_<1'000'000,         "MB"> {};
  struct gigabytes : bytes_<1'000'000'000,     "GB"> {};
  struct terabytes : bytes_<1'000'000'000'000, "TB"> {};

  struct best_size_unit : bytes_base {
    std::size_t factor;
    std::string_view units;

    constexpr best_size_unit(std::size_t n_bytes) : bytes_base(n_bytes) {
      // wtf is this
      constexpr std::tuple<bytes, kibibytes, mebibytes, gibibytes, tebibytes> sizes {};
      
      [&]<auto... I>(std::index_sequence<I...>) {
        (void)(((n_bytes >= std::get<I>(sizes).factor)
                ? (factor = std::get<I>(sizes).factor, units = std::get<I>(sizes).units, true) : false) && ...);
      } (std::make_index_sequence<std::tuple_size_v<decltype(sizes)>>());
    }

    constexpr std::string string() const {
      return fmt::format("{:.1f} {}", static_cast<double>(n) / factor, units);
    }
  };
}

template <std::derived_from<cdb::bytes_base> B, typename CharT>
struct fmt::formatter<B, CharT> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& pctx) {
    return pctx.begin();
  }

  template <typename FormatContext>
  auto format(const B &b, FormatContext &fctx) const {
    return fmt::format_to(fctx.out(), "{} {}", b.n / b.factor, b.units);
  }
};
