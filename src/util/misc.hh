#pragma once

#include <type_traits>

namespace cdb {

template <typename T>
concept Numeric = requires(T t)
{
  requires std::is_integral_v<T> || std::is_floating_point_v<T>;
  requires std::is_arithmetic_v<decltype(t + 1)>;
  requires !std::is_same_v<bool, T>;
  requires !std::is_pointer_v<T>;
};

template <class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

} // cdb
