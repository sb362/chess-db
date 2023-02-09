#pragma once

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <semaphore>
#include <stop_token>
#include <thread>

#include "util/vector.hh"

namespace cdb::async {

template <std::movable T>
class queue_with_lock {
private:
  std::deque<T> _deque;
  std::mutex _mutex;

public:
  queue_with_lock() : _deque(), _mutex() {}

  void push(T &&value) {
    std::unique_lock lock {_mutex};
    _deque.push_back(value);
  }

  std::optional<T> pop() {
    std::unique_lock lock {_mutex};
    if (_deque.empty())
      return std::nullopt;

    std::optional<T> value {std::move(_deque.front())};
    _deque.pop_front();
    return value;
  }

  void clear() {
    std::unique_lock lock {_mutex};
    _deque.clear();
  }
};

class thread_pool {
private:
  using Task = std::function<void ()>;

  struct worker {
    worker(auto &&t) : queue(), active(0), thread(t) {}

    queue_with_lock<Task> queue;
    std::binary_semaphore active;
    std::jthread thread;
  };

  cdb::stable_vector<worker, 4> workers;
  std::atomic<std::size_t> pending;
  std::size_t next_id;

  void stop() {
    for (auto &w : workers) {
      w.thread.request_stop();
      w.active.release();
      w.thread.join();
    }
  }

public:
  thread_pool(size_t n = std::thread::hardware_concurrency())
    : workers(), pending(0), next_id(0)
  {
    resize(n);
  }

  ~thread_pool() {
    stop();
  }

  thread_pool(const thread_pool &) = delete;
  thread_pool &operator=(const thread_pool &) = delete;

  void resize(size_t n) {
    stop();
    workers.clear();
  
    for (std::size_t id = 0; id < n; ++id) {
      workers.emplace_back([this, id] (const std::stop_token &stop_token) {
        auto run = [this] (Task &&t) {
          pending.fetch_sub(1, std::memory_order_release);
          std::invoke(t);
        };

        while (!stop_token.stop_requested()) {
          workers[id].active.acquire();

          while (auto p = pending.load(std::memory_order_acquire)) {
            while (auto task = workers[id].queue.pop())
              run(std::move(*task));

            for (std::size_t i = id + 1; i != id; ++i %= workers.size()) {
              if (auto task = workers[id].queue.pop()) {
                run(std::move(*task));
                break;
              }
            }
          }
        }
      });
    }
  }

  template <class F>
  void push(F &&func) {
    workers[next_id].queue.push(std::forward<F>(func));
    workers[next_id].active.release();
    next_id += 1;
    next_id %= workers.size();
    ++pending;
  }

  template <class F, class ...Args>
  void push(F &&func, Args &&...args) {
    push([func = std::forward<F>(func), ...args = std::forward<Args>(args)] { func(args...); } );
  }
};

} // cdb::async

