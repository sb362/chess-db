#pragma once

#include <algorithm>
#include <array>
#include <numeric>
#include <ranges>
#include <span>

#include <fmt/format.h>

namespace cdb::util {
  struct Histogram {
    std::array<std::size_t, 256> counts {};
    double bits_per_symbol = 0;

    template <std::size_t S>
    constexpr Histogram(std::span<const std::byte, S> ss) {
      
      for (const std::byte &b : ss) {
        counts[static_cast<unsigned>(b)] += 1;
      }
    }

    void print() {
      constexpr std::string_view s = "********************************************************************************";

      auto largest = std::ranges::max(counts);
      auto total = std::accumulate(counts.begin(), counts.end(), 0ull);
      //auto bits_per_symbol = 0.;

      int i, j;
      for (i = 0, j = 0; i < counts.size(); ++i) {
        auto count = static_cast<double>(counts[i]);
        auto prob = count / total;
        auto entropy = -prob * std::log2(prob);

        auto length = static_cast<std::size_t>((count / largest) * s.size());

        if (i > 0 && length > 0) {
          if (j)
            fmt::print("{:2d} - {:2d}\n", std::exchange(j, 0), i - 1);

          fmt::print("{:2d} {:.3f} {}\n", i, entropy, s.substr(0, length));
        } else
          j = i;
        
        if (prob > 0)
          bits_per_symbol += entropy;
      }

      if (j)
        fmt::print("{:2d} - {:2d}\n", std::exchange(j, 0), i - 1);

      fmt::print("bits per symbol: {:.3f}\n", bits_per_symbol);

      for (int i = 0; i < counts.size(); ++i)
        fmt::print("{},", counts[i]);

      fmt::print("\n");
    }
  };
} // cdb
