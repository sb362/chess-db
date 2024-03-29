#pragma once

#include <iterator>

namespace cdb {

template <class This, class Value, class Tag = std::forward_iterator_tag>
struct iterator_facade {
  using value_type = Value;
  using reference  = value_type &;
  using pointer    = value_type *;
  using iterator_category = Tag;

  This &self() { return static_cast<This &>(*this); }
  This const &self() const { return static_cast<This const &>(*this); }

  Value &operator*() const { return self().value(); }
  Value *operator->() const { return std::addressof(**this); }

  This &operator++() {
    self().increment();
    return self();
  }

  This operator++(int) {
    auto s = self();
    self().increment();
    return s;
  }

  bool operator==(This const &other) const { return self().equal(other); }
  bool operator!=(This const &other) const { return !(*this == other); }
};

} // cdb
