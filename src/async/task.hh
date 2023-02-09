#pragma once

#include <atomic>
#include <concepts>
#include <coroutine>
#include <optional>
#include <type_traits>
#include <utility>

namespace cdb::async {

enum class task_status {
  none = 0,
  running,
  finished
};

template <std::movable T>
struct task final {
  struct promise_type {
    task get_return_object() { return handle_type::from_promise(*this); }

    constexpr std::suspend_always initial_suspend() noexcept { return {}; }
    constexpr std::suspend_always final_suspend() noexcept { return {}; }
    
    constexpr std::suspend_always yield_value() = delete;

    constexpr void await_transform() = delete;
    void unhandled_exception() { throw; }

    template <std::convertible_to<T> From>
    constexpr void return_value(From &&value) { value = std::forward<From>(value); }

    std::optional<T> value;
  };

  using handle_type = std::coroutine_handle<promise_type>;

  task(handle_type ch) : _ch(ch) {}
  ~task() { destroy(); }
  
  void destroy() noexcept { if (_ch) _ch.destroy(); }
  
  task(const task &) = delete;
  task &operator=(const task &) = delete;

  task(task &&other) noexcept : _ch(std::move(other._ch)) {}
  task &operator=(task &&other) noexcept {
    if (this != std::addressof(other)) {
      destroy();
      _ch = std::exchange(other._ch, {});
    }

    return *this;
  }

  void run() {
    auto status = task_status::none;
    if (_status.compare_exchange_strong(status, task_status::running)) {
      _ch.resume();
      _status.exchange(task_status::finished);
    }
  }

  auto operator()() { run(); return std::move(*_ch.promise().value); }
  explicit operator bool() { return _ch.done(); }

private:
  handle_type _ch;
  std::atomic<task_status> _status;
};

} // cdb::async
