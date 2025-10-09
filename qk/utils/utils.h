#ifndef UTILS_H
#define UTILS_H

#ifdef QK_UTILS

#include <concepts>
#include <optional>
#include <utility>

/// implements miscellaneous small utilities not deserving of their own modules
namespace qk::utils {

/// similar to 'std::scope_exit', more go like and simpler, designed to be used with the 'defer'
/// macros
template <std::invocable Func>
struct ScopeGuard {
    std::optional<Func> _func;

    void cancel() noexcept { _func.reset(); }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    ScopeGuard(ScopeGuard&& other) noexcept : _func(std::move(other._func)) {}
    ScopeGuard& operator=(ScopeGuard&& other) noexcept {
        _func = std::move(other._func);
        return *this;
    }

    explicit ScopeGuard(Func&& func) : _func(std::forward<Func>(func)) {}
    ~ScopeGuard() noexcept {
        if (_func) (*_func)();
    }
};

/// defines a defer executing when the current scope ends, the provided expression will be executed
/// in a lambda capturing all variables by reference, if you need capturing by value use
/// 'defer_val', or 'defer_raw' if you want to use a function pointer or another invokable object
#define defer(func) qk::utils::ScopeGuard qk_defer_##__LINE__([&] { func; })

/// same as 'defer' but captures variables by value instead of reference
#define defer_val(func) qk::utils::ScopeGuard qk_defer_##__LINE__([=] { func; })

/// same as 'defer' and 'defer_val', does not automatically wrap the provided code in a lambda, just
/// passes it directly to the scope guard
#define defer_raw(func) qk::utils::ScopeGuard qk_defer_##__LINE__(func)

}  // namespace qk::utils

#endif

#endif  // UTILS_H
