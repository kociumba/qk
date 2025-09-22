#ifndef GORUTINES_H
#define GORUTINES_H

#ifdef QK_THREADING

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include "../api.h"

namespace qk::threading {

template <typename Func, typename... Args>
QK_API void go(Func&& func, Args&&... args) {
    std::jthread([func = std::forward<Func>(func), ... args = std::forward<Args>(args)]() mutable {
        func(std::forward<Args>(args)...);
    }).detach();
}

template <typename T>
struct QK_API channel {
    std::queue<T> _queue;
    size_t _capacity = 0;
    std::mutex _mu;
    std::condition_variable _not_empty, _not_full;
    std::atomic_bool _closed = false;

    std::atomic<int> _senders_waiting = 0;
    std::atomic<int> _receivers_waiting = 0;

    channel(size_t capacity = 0) : _capacity(capacity) {}

    channel(channel&& other) noexcept
        : _queue(std::move(other._queue)),
          _capacity(other._capacity),
          _closed(other._closed.load()),
          _senders_waiting(other._senders_waiting.load()),
          _receivers_waiting(other._receivers_waiting.load()) {}

    channel& operator=(channel&& other) noexcept {
        if (this != &other) {
            std::lock_guard lock(_mu);
            _queue = std::move(other._queue);
            _capacity = other._capacity;
            _closed = other._closed.load();
            _senders_waiting = other._senders_waiting.load();
            _receivers_waiting = other._receivers_waiting.load();
        }
        return *this;
    }

    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;

    bool send(const T& val) {
        std::unique_lock l(_mu);
        if (_closed.load()) return false;

        if (_capacity == 0) {
            ++_senders_waiting;
            _not_empty.notify_one();

            _not_full.wait(l, [this] { return _closed.load() || _receivers_waiting.load() > 0; });
            --_senders_waiting;

            if (_closed.load()) return false;

            _queue.push(val);
            _not_empty.notify_one();
            return true;
        }

        _not_full.wait(l, [this] { return _closed.load() || _queue.size() < _capacity; });

        if (_closed.load()) return false;

        _queue.push(val);
        _not_empty.notify_one();
        return true;
    }

    bool send(T&& val) {
        std::unique_lock l(_mu);
        if (_closed.load()) return false;

        if (_capacity == 0) {
            ++_senders_waiting;
            _not_empty.notify_one();

            _not_full.wait(l, [this] { return _closed.load() || _receivers_waiting.load() > 0; });

            --_senders_waiting;

            if (_closed.load()) return false;

            _queue.push(std::move(val));
            _not_empty.notify_one();
            return true;
        }

        _not_full.wait(l, [this] { return _closed.load() || _queue.size() < _capacity; });

        if (_closed.load()) return false;

        _queue.push(std::move(val));
        _not_empty.notify_one();
        return true;
    }

    std::optional<T> receive() {
        std::unique_lock l(_mu);

        if (_capacity == 0) {
            ++_receivers_waiting;
            _not_full.notify_one();

            _not_empty.wait(l, [this] {
                return !_queue.empty() || (_closed.load() && _senders_waiting.load() == 0);
            });

            --_receivers_waiting;

            if (_queue.empty() && _closed.load()) {
                return std::nullopt;
            }

            T val = std::move(_queue.front());
            _queue.pop();
            return val;
        }

        _not_empty.wait(l, [this] { return !_queue.empty() || _closed.load(); });

        if (_queue.empty() && _closed.load()) {
            return std::nullopt;
        }

        T val = std::move(_queue.front());
        _queue.pop();
        _not_full.notify_one();
        return val;
    }

    void close() {
        std::lock_guard l(_mu);
        _closed = true;
        _not_empty.notify_all();
        _not_full.notify_all();
    }

    bool is_closed() const { return _closed.load(); }

    channel& operator<<(const T& val) {
        send(val);
        return *this;
    }

    channel& operator<<(T&& val) {
        send(std::move(val));
        return *this;
    }

    channel& operator>>(T& val) {
        if (auto opt_val = receive()) {
            val = std::move(*opt_val);
        }
        return *this;
    }

    struct iterator {
        channel* ch;
        std::optional<T> current;
        iterator(channel* c) : ch(c) {
            if (ch) current = ch->receive();
        }
        iterator() : ch(nullptr) {}
        T& operator*() { return *current; }
        T* operator->() { return &(*current); }
        iterator& operator++() {
            if (ch) current = ch->receive();
            if (!current) ch = nullptr;
            return *this;
        }
        bool operator!=(const iterator& other) const { return ch != other.ch; }
    };

    iterator begin() { return iterator(this); }
    iterator end() { return iterator(); }
};

using int_channel = channel<int>;
using uint_channel = channel<unsigned int>;
using float_channel = channel<float>;
using double_channel = channel<double>;
using string_channel = channel<std::string>;
using bool_channel = channel<bool>;

}  // namespace qk::threading

#endif

#endif  // GORUTINES_H
