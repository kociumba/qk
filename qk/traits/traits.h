#ifndef TRAITS_H
#define TRAITS_H

#ifdef QK_TRAITS

#include <concepts>
#include <functional>
#include <mutex>

/// this namespace provides trait/impl like functionality for specific utilities
/// all of these are compile time only, and do not introduce any runtime overhead
///
/// the idea here is that we use concepts as traits, and base structs as default implementations
/// these base structs are meant to be publicly inherited from to satisfy the "traits"
namespace qk::traits {

template <typename T>
concept Copyable = std::copy_constructible<T> && std::is_copy_assignable_v<T>;

template <typename T>
concept Movable = std::movable<T>;

template <typename T>
concept RefCounted = requires(T& t) {
    { t.increment() } -> std::same_as<void>;
    { t.decrement() } -> std::same_as<bool>;
};

struct RefCounted_base {
    size_t _ref = 1;

    void increment() noexcept { ++_ref; }
    bool decrement() noexcept {
        if (_ref == 0) [[unlikely]]
            return false;
        --_ref;
        return _ref != 0;
    }
};

static_assert(RefCounted<RefCounted_base>);

template <typename T>
concept Lockable = requires(T& t) {
    { t.lock() } -> std::same_as<void>;
    { t.unlock() } -> std::same_as<void>;
    { t.mu() } -> std::convertible_to<std::mutex&>;
};

struct Lockable_base {
    std::mutex _m;

    void lock() noexcept { _m.lock(); }
    void unlock() noexcept { _m.unlock(); }
    std::mutex& mu() noexcept { return _m; }
};

static_assert(Lockable<Lockable_base>);

/// the default Hashable implementation is not replicable, based on the memory address of the
/// inheriting structure, if you need hashes of the value of structures, use ValueHashable from
/// trait extras
template <typename T>
concept Hashable = requires(const T& t) {
    { t.hash() } -> std::convertible_to<size_t>;
};

inline std::size_t _combine_hash(std::size_t seed, std::size_t v) noexcept {
    return seed ^ v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

/// not consistent on apple arm, due to libc++ not containing pointer hashing
struct Hashable_base {
    template <class Self>
    [[nodiscard]] size_t hash(this Self const& self) noexcept {
#if defined(__APPLE__) && defined(__arm64__)
        auto p = reinterpret_cast<std::uintptr_t>(&self);
        return _combine_hash(p, 1469598103934665603ULL);
#else
        return std::hash<const void*>{}(static_cast<const void*>(&self));
#endif
    }
};

static_assert(Hashable<Hashable_base>);

}  // namespace qk::traits

#endif

#endif  // TRAITS_H
