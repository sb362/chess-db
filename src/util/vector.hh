#pragma once

#include <array>
#include <cassert>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace cdb {

template <typename T, std::size_t Size>
class static_vector {
  using storage_type = std::array<std::byte, Size * sizeof(T)>;

public:
  using value_type             = T;
  using size_type              = std::size_t;
  using difference_type        = std::ptrdiff_t;
  using reference              = T &;
  using const_reference        = const T &;
  using pointer                = T *;
  using const_pointer          = const T *;

private:
  alignas(T) storage_type _data;
  size_type _n = 0;

  constexpr auto data() { return reinterpret_cast<T *>(_data.data()); }
  constexpr auto data() const { return reinterpret_cast<const T *>(_data.data()); }

public:
  constexpr static_vector() = default;
  constexpr ~static_vector() { clear(); }

  // todo: idk what I'm doing here..
  constexpr static_vector(const static_vector &v) requires std::is_copy_constructible_v<T> {
    for (const auto &e : v)
      emplace_back(e);
  }

  constexpr static_vector(static_vector &&v) noexcept requires std::is_nothrow_move_constructible_v<T> {
    for (auto &&e : v)
      emplace_back(std::move(e));
  }

  constexpr static_vector &operator=(const static_vector &v) requires std::is_copy_assignable_v<T> {
    for (const auto &e : v)
      emplace_back(e);

    return *this;
  }

  constexpr static_vector &operator=(static_vector &&v) noexcept requires std::is_nothrow_move_assignable_v<T> {
    for (auto &&e : v)
      emplace_back(std::move(e));

    return *this;
  }

  constexpr size_type size() const noexcept { return _n; }
  constexpr size_type capacity() const noexcept { return Size; }

  constexpr size_type remaining() const noexcept { return capacity() - size(); }

  constexpr void push_back(const value_type &value) { emplace_back(value); };
  constexpr void push_back(value_type &&value) { emplace_back(std::move(value)); };

  template <class ...Args>
  constexpr decltype(auto) emplace_back(Args &&...args) {
    assert(_n < capacity());
    return *std::construct_at(data() + _n++, std::forward<Args>(args)...);
  }

  constexpr void pop_back() noexcept {
    assert(_n);
    std::destroy_at(data() + _n--);
  }

  constexpr reference back() { return operator[](_n - 1); }
  constexpr const_reference back() const { return operator[](_n - 1); }

  constexpr reference front() { return operator[](0); }
  constexpr const_reference front() const { return operator[](0); }

  constexpr reference operator[](size_type i) noexcept { assert(i < _n); return *(data() + i); }
  constexpr const_reference operator[](size_type i) const noexcept { assert(i < _n); return *(data() + i); }

  template <typename U>
  struct base_iterator {
    using iterator_category = std::contiguous_iterator_tag; // necessary?

    U *_ptr;

    constexpr base_iterator(U *ptr) noexcept
      : _ptr(ptr) {}

    constexpr U &operator*() const { return *_ptr; }
    constexpr U *operator->() const { return _ptr; }

    constexpr base_iterator &operator++() { ++_ptr; return *this; }
    constexpr base_iterator &operator++(int) { auto tmp = *this; ++*this; return tmp; }

    constexpr bool operator==(const base_iterator &b) const { return _ptr == b._ptr; }
    constexpr bool operator!=(const base_iterator &b) const { return _ptr != b._ptr; }
  };

  using iterator = base_iterator<value_type>;
  using const_iterator = base_iterator<const value_type>;

  constexpr iterator begin() noexcept { return iterator(data()); }
  constexpr iterator end() noexcept { return iterator(data() + size()); }

  constexpr const_iterator begin() const noexcept { return const_iterator(data()); }
  constexpr const_iterator end() const noexcept { return const_iterator(data() + size()); }

  constexpr const_iterator cbegin() const noexcept { return const_iterator(data()); }
  constexpr const_iterator cend() const noexcept { return const_iterator(data() + size()); }

  constexpr void clear() noexcept {
    while (_n)
      std::destroy_at(data() + --_n);
  };
};

template <class T, std::size_t ChunkSize>
class stable_vector {
  static_assert((ChunkSize & (ChunkSize - 1)) == 0, "Chunk size must be a power of two");
  using chunk_type = static_vector<T, ChunkSize>;
  using chunk_ptr = std::unique_ptr<chunk_type>;

  std::vector<chunk_ptr> _chunks {};

public:
  using value_type             = typename chunk_type::value_type;
  using size_type              = typename chunk_type::size_type;
  using difference_type        = typename chunk_type::difference_type;
  using reference              = typename chunk_type::reference;
  using const_reference        = typename chunk_type::const_reference;
  using pointer                = typename chunk_type::pointer;
  using const_pointer          = typename chunk_type::const_pointer;

private:
  bool _needs_new_chunk() const noexcept {
    return _chunks.empty() || _chunks.back()->remaining() == 0;
  }

  void _alloc_chunk_if_needed() {
    if (_needs_new_chunk())
      _chunks.push_back(std::make_unique<chunk_type>());
  }

  void _dealloc_last_chunk_if_empty() {
    if (_chunks.back()->size() == 0)
      _chunks.pop_back();
  }

public:
  class iterator {
    size_type _pos;
    stable_vector *const _sv;

  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = stable_vector::difference_type;
    using value_type        = stable_vector::value_type;
    using pointer           = stable_vector::pointer;
    using reference         = stable_vector::reference;

    constexpr iterator(size_type pos, stable_vector *sv) : _pos(pos), _sv(sv) {}

    reference operator*() noexcept { return _sv->operator[](_pos); }
    pointer operator->() noexcept { return _sv->operator[](_pos); }

    iterator &operator++() noexcept { ++_pos; return *this; }
    iterator operator++(int) noexcept { auto t = *this; operator++(); return t; }

    friend bool operator==(const iterator &a, const iterator &b) { return a._pos == b._pos && a._sv == b._sv; }
    friend bool operator!=(const iterator &a, const iterator &b) { return !(a == b); }
  };

public:
  constexpr stable_vector() = default;

  size_type capacity() const noexcept {
    return _chunks.empty() ? 0 : (_chunks.size() * ChunkSize + _chunks.back()->size());
  }

  size_type size() const noexcept {
    return _chunks.empty() ? 0 : ((_chunks.size() - 1) * ChunkSize + _chunks.back()->size());
  }

  void push_back(const value_type &value) { emplace_back(value); };
  void push_back(value_type &&value) { emplace_back(std::move(value)); };

  template <class ...Args>
  decltype(auto) emplace_back(Args &&...args) {
    _alloc_chunk_if_needed();
    return _chunks.back()->emplace_back(std::forward<Args>(args)...);
  }

  void pop_back() noexcept {
    assert(!_chunks.empty());
    _chunks.back()->pop_back();
    _dealloc_last_chunk_if_empty();
  }

  void clear() {
    _chunks.clear();
  }

  reference       operator[](size_type i) noexcept { return (*_chunks[i / ChunkSize])[i % ChunkSize]; }
  const_reference operator[](size_type i) const noexcept {  return (*_chunks[i / ChunkSize])[i % ChunkSize]; }

  iterator begin() { return iterator(0, this); }
  iterator end() { return iterator(size(), this); }
};

} // cdb
