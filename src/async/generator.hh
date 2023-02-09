#pragma once

#include <concepts>
#include <coroutine>
#include <exception>
#include <optional>
#include <utility>

namespace cdb::async {

template <std::movable T>
struct generator {
  struct promise_type {
    generator get_return_object() { return handle_type::from_promise(*this); }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    
    template <std::convertible_to<T> From>
    std::suspend_always yield_value(From &&from) {
      value = std::forward<From>(from);
      return {};
    }

    void await_transform() = delete;
    void unhandled_exception() { except = std::current_exception(); std::cout << "exceptino!\n"; }
    void return_void() {}

    std::optional<T> value;
    std::exception_ptr except;
  };

  using handle_type = std::coroutine_handle<promise_type>;

  generator(handle_type ch) : _ch(ch) {}
  ~generator() { destroy(); }
  
  void destroy() noexcept { if (_ch) _ch.destroy(); }
  
  generator(const generator &) = delete;
  generator &operator=(const generator &) = delete;

  generator(generator &&other) noexcept : _ch(std::move(other._ch)) {}
  generator &operator=(generator &&other) noexcept {
    if (this != std::addressof(other)) {
      destroy();
      _ch = std::exchange(other._ch, {});
    }

    return *this;
  }

  void run() {
    if (!_ch.promise().value.has_value()) {
      _ch();

      if (_ch.promise().except)
        std::rethrow_exception(_ch.promise().except);
    }
  }

  T operator()() { run(); return *std::exchange(_ch.promise().value, std::nullopt); }
  explicit operator bool() { run(); return !_ch.done(); }

private:
  handle_type _ch;
};

} // cdb::async
