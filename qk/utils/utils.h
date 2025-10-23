#ifndef UTILS_H
#define UTILS_H

#ifdef QK_UTILS

#include <algorithm>
#include <concepts>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <ranges>
#include <source_location>
#include <span>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "../api.h"

#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
#define DEBUG_BREAK() __builtin_trap()
#else
#define DEBUG_BREAK() std::abort()
#pragma message("QK_ALWAYS_ASSERT will not be able to trigger a debug trap on this compiler")
#endif

/// simple always assert, used internally
///
/// can be used externally to achieve an assert that is not disabled in release builds
#define QK_ALWAYS_ASSERT(expr)                                                              \
    do {                                                                                    \
        if (!(expr)) {                                                                      \
            std::cerr << "FALSE ASSERT: " << #expr << " at " << __FILE__ << ":" << __LINE__ \
                      << std::endl;                                                         \
            DEBUG_BREAK();                                                                  \
            std::abort();                                                                   \
        }                                                                                   \
    } while (0)

/// implements miscellaneous small utilities not deserving of their own modules
namespace qk::utils {

/// similar to 'std::scope_exit', more go like and simpler, designed to be used with the 'defer'
/// macros
template <std::invocable Func>
struct QK_API ScopeGuard {
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

/// generic stream type, thin wrapper around a std::vector to simplify stream like operations
///
/// the implementation is optimized to introduce as little overhead over using std::vector as
/// possible
///
/// a stream can also accept ranges, and the last element can be popped off using --
///
/// stream also implicitly converts to T*, this is done to simplify using stream in place of classic
/// array operations
template <typename T>
struct stream {
    std::vector<T> data;

    stream() = default;
    stream(size_t size) : data(size) {}
    stream(T* begin, T* end) : data(begin, end) {}

    template <std::ranges::input_range R>
        requires(!std::same_as<std::remove_cvref_t<R>, stream>)
    stream(R&& range) {
        if constexpr (std::is_rvalue_reference_v<R&&>) {
            data = std::vector<T>(
                std::make_move_iterator(std::ranges::begin(range)),
                std::make_move_iterator(std::ranges::end(range))
            );
        } else {
            data = std::vector<T>(std::ranges::begin(range), std::ranges::end(range));
        }
    }
    stream(std::from_range_t, auto&& range) : data(std::forward<decltype(range)>(range)) {}

    stream(std::span<const T> span) : data(span.begin(), span.end()) {}
    stream(std::span<T> span) : data(span.begin(), span.end()) {}

    stream(stream&& other) noexcept = default;
    stream& operator=(stream&& other) noexcept = default;
    stream(const stream& other) = default;
    stream& operator=(const stream& other) = default;

    friend stream& operator<<(stream& s, T&& value) {
        s.data.emplace_back(std::forward<T>(value));
        return s;
    }

    friend stream& operator<<(stream& s, const T& value) {
        s.data.push_back(value);
        return s;
    }

    template <std::ranges::input_range R>
    stream& operator<<(R&& range) {
        data.reserve(data.size() + std::ranges::size(range));
        if constexpr (std::is_rvalue_reference_v<R&&>) {
            std::ranges::move(range, std::back_inserter(data));
        } else {
            std::ranges::copy(range, std::back_inserter(data));
        }
        return *this;
    }

    stream& operator--() {
        data.pop_back();
        return *this;
    }

    friend T* operator~(stream& s) noexcept { return s.data.data(); }
    friend const T* operator~(const stream& s) noexcept { return s.data.data(); }

    operator T*() noexcept { return data.data(); }
    operator const T*() const noexcept { return data.data(); }

    [[nodiscard]] T& operator[](std::size_t pos) { return data[pos]; }
    [[nodiscard]] const T& operator[](std::size_t pos) const { return data[pos]; }

    [[nodiscard]] size_t size() const { return data.size(); }
    [[nodiscard]] bool empty() const noexcept { return data.empty(); }

    [[nodiscard]] T* render() noexcept { return data.data(); }
    [[nodiscard]] const T* render() const noexcept { return data.data(); }

    [[nodiscard]] std::span<T> span() noexcept { return data; }
    [[nodiscard]] std::span<const T> span() const noexcept { return data; }

    void reserve(size_t n) { data.reserve(n); }
    [[nodiscard]] size_t capacity() const { return data.capacity(); }
    void compact() { data.shrink_to_fit(); }
    void clear() noexcept { data.clear(); }

    auto begin() { return data.begin(); }
    auto begin() const { return data.begin(); }
    auto end() { return data.end(); }
    auto end() const { return data.end(); }

    // fancy features
    template <std::invocable Func>
        requires std::same_as<std::invoke_result_t<Func>, bool>
    void filter(Func&& func) {
        std::erase_if(data, [&](const T& item) { return !func(item); });
        this->compact();
    }

    template <std::invocable Func>
    auto map(Func&& func) {
        using Res = std::invoke_result_t<Func, decltype(this->begin())>;
        stream<Res> result;
        result.reserve(this->size());
        for (auto&& x : *this) {
            result << func(x);
        }

        return result;
    }
};

struct SourceLocationEqual {
    bool operator()(
        const std::source_location& lhs, const std::source_location& rhs
    ) const noexcept {
        if (std::string_view(lhs.file_name()) != std::string_view(rhs.file_name())) return false;

        if (lhs.line() != rhs.line()) return false;
        if (lhs.column() != rhs.column()) return false;

        if (std::string_view(lhs.function_name()) != std::string_view(rhs.function_name()))
            return false;

        return true;
    }
};

struct SourceLocationHash {
    std::size_t operator()(const std::source_location& loc) const noexcept {
        std::size_t seed = std::hash<std::string_view>{}(std::string_view{loc.file_name()});

        seed ^= std::hash<std::uint_least32_t>{}(loc.line()) + 1469598103934665603ULL +
                (seed << 6) + (seed >> 2);

        seed ^= std::hash<std::uint_least32_t>{}(loc.column()) + 1469598103934665603ULL +
                (seed << 6) + (seed >> 2);

        seed ^= std::hash<std::string_view>{}(std::string_view{loc.function_name()}) +
                1469598103934665603ULL + (seed << 6) + (seed >> 2);

        return seed;
    }
};

/// runs a provided invocable only once per source location
///
/// it is recommended to only use no argument and return lambdas here, since the static storage
/// will be duplicated for each invokable type passed to once in a program
///
/// @note
/// while this function is thread safe and guarded with a mutex, using it in a multithreaded
/// scenario as a mechanism to trigger actions only once is a very bad idea
template <std::invocable Func>
void once(Func&& func, std::source_location loc = std::source_location::current()) {
    static std::unordered_set<std::source_location, SourceLocationHash, SourceLocationEqual>
        registry;
    static std::mutex mu;

    // std::stringstream ss;
    // ss << loc.file_name() << loc.line() << loc.column();
    // const auto id = ss.str();

    {
        std::lock_guard l(mu);
        if (!registry.contains(loc)) {
            registry.insert(loc);
            func();
        }
    }
}

template <typename T>
struct Ok {
    T value;
    explicit Ok(T&& val) : value(std::forward<T>(val)) {}
};

template <typename T>
struct Err {
    T value;
    explicit Err(T&& val) : value(std::forward<T>(val)) {}
};

template <typename T>
Ok<std::decay_t<T>> ok(T&& value) {
    return Ok<std::decay_t<T>>{std::forward<T>(value)};
}

template <typename T>
Err<std::decay_t<T>> err(T&& value) {
    return Err<std::decay_t<T>>{std::forward<T>(value)};
}

/// a rust style result, prioritising usage ergonomics, since that is the downfall of rusts result
///
/// a result of <OK, ERR> can be implicitly constructed from those types, example:
///     @code
///     Result<int, std::string> func() { return "error string"; }
///     @endcode
///
/// and can be statically cast to OK or ERR based on the held value, example:
///     @code
///     auto r = func();
///     if (!r) error_handler(static_cast<std::string>(r));
///     @endcode
template <typename OK, typename ERR>
struct Result {
    union {
        OK ok_val;
        ERR err_val;
    };
    bool is_ok;

    ~Result() {
        if (is_ok) {
            ok_val.~OK();
        } else {
            err_val.~ERR();
        }
    }

    template <typename T>
    Result(Ok<T>&& ok) {
        static_assert(std::is_constructible_v<OK, T>, "Ok value must be constructible to OK type");
        std::construct_at(&ok_val, std::forward<T>(ok.value));
        is_ok = true;
    }

    template <typename T>
    Result(Err<T>&& err) {
        static_assert(
            std::is_constructible_v<ERR, T>, "Err value must be constructible to ERR type"
        );
        std::construct_at(&err_val, std::forward<T>(err.value));
        is_ok = false;
    }

    Result(const OK& ok) : ok_val(ok), is_ok(true) {}
    Result(OK&& ok) : ok_val(std::move(ok)), is_ok(true) {}

    Result(const ERR& err) : err_val(err), is_ok(false) {}
    Result(ERR&& err) : err_val(std::move(err)), is_ok(false) {}

    Result(const Result& other) : is_ok(other.is_ok) {
        if (is_ok)
            std::construct_at(&ok_val, other.ok_val);
        else
            std::construct_at(&err_val, other.err_val);
    }

    Result(
        Result&& other
    ) noexcept(std::is_nothrow_move_constructible_v<OK> && std::is_nothrow_move_constructible_v<ERR>)
        : is_ok(other.is_ok) {
        if (is_ok)
            std::construct_at(&ok_val, std::move(other.ok_val));
        else
            std::construct_at(&err_val, std::move(other.err_val));
    }

    Result& operator=(const Result& other) {
        if (this != &other) {
            this->~Result();
            std::construct_at(this, other);
        }
        return *this;
    }

    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            this->~Result();
            std::construct_at(this, std::move(other));
        }
        return *this;
    }

    template <typename T>
        requires(
            !std::same_as<std::decay_t<T>, Result> &&
            ((std::constructible_from<OK, T> && !std::constructible_from<ERR, T>) ||
             (std::constructible_from<ERR, T> && !std::constructible_from<OK, T>))
        )
    Result(T&& value) {
        if constexpr (std::is_constructible_v<OK, T>) {
            std::construct_at(&ok_val, std::forward<T>(value));
            is_ok = true;
        } else if constexpr (std::is_constructible_v<ERR, T>) {
            std::construct_at(&err_val, std::forward<T>(value));
            is_ok = false;
        } else
            static_assert(sizeof(T) == 0, "T must be convertible to OK or ERR");
    }

    explicit operator OK&() & {
        QK_ALWAYS_ASSERT(is_ok && "Attempted to convert error Result to OK type");
        return ok_val;
    }

    explicit operator const OK&() const& {
        QK_ALWAYS_ASSERT(is_ok && "Attempted to convert error Result to OK type");
        return ok_val;
    }

    explicit operator OK&&() && {
        QK_ALWAYS_ASSERT(is_ok && "Attempted to convert error Result to OK type");
        return std::move(ok_val);
    }

    explicit operator ERR&() & {
        QK_ALWAYS_ASSERT(!is_ok && "Attempted to convert OK Result to ERR type");
        return err_val;
    }

    explicit operator const ERR&() const& {
        QK_ALWAYS_ASSERT(!is_ok && "Attempted to convert OK Result to ERR type");
        return err_val;
    }

    explicit operator ERR&&() && {
        QK_ALWAYS_ASSERT(!is_ok && "Attempted to convert OK Result to ERR type");
        return std::move(err_val);
    }

    friend bool operator==(const Result& a, const Result& b)
        requires std::equality_comparable<OK> && std::equality_comparable<ERR>
    {
        if (a.is_ok != b.is_ok) return false;
        return a.is_ok ? (a.ok_val == b.ok_val) : (a.err_val == b.err_val);
    }

    [[nodiscard]] bool ok() const noexcept { return is_ok; }
    [[nodiscard]] bool err() const noexcept { return !is_ok; }

    OK& unwrap() & {
        QK_ALWAYS_ASSERT(is_ok && "Unwrapped an error Result");
        return ok_val;
    }
    const OK& unwrap() const& {
        QK_ALWAYS_ASSERT(is_ok && "Unwrapped an error Result");
        return ok_val;
    }
    OK&& unwrap() && {
        QK_ALWAYS_ASSERT(is_ok && "Unwrapped an error Result");
        return std::move(ok_val);
    }

    ERR& unwrap_err() & {
        QK_ALWAYS_ASSERT(!is_ok && "Unwrapped an OK Result");
        return err_val;
    }
    const ERR& unwrap_err() const& {
        QK_ALWAYS_ASSERT(!is_ok && "Unwrapped an OK Result");
        return err_val;
    }
    ERR&& unwrap_err() && {
        QK_ALWAYS_ASSERT(!is_ok && "Unwrapped an OK Result");
        return std::move(err_val);
    }

    OK unwrap_or(OK&& default_val) && {
        return is_ok ? std::move(ok_val) : std::forward<OK>(default_val);
    }
    const OK& unwrap_or(const OK& default_val) const& { return is_ok ? ok_val : default_val; }

    template <typename Func>
    auto map(Func&& func) && -> Result<std::invoke_result_t<Func, OK>, ERR> {
        if (is_ok) return std::invoke(std::forward<Func>(func), std::move(ok_val));
        return std::move(err_val);
    }

    template <typename Func>
    auto and_then(Func&& func) && -> std::invoke_result_t<Func, OK> {
        static_assert(
            std::is_same_v<ERR, typename std::invoke_result_t<Func, OK>::ERR>,
            "Error types must match"
        );
        if (is_ok) return std::invoke(std::forward<Func>(func), std::move(ok_val));
        return std::move(err_val);
    }

    template <typename U>
    OK value_or(U&& default_val) const& {
        return is_ok ? ok_val : static_cast<OK>(std::forward<U>(default_val));
    }

    template <std::invocable Func>
        requires std::invocable<Func, ERR>
    auto map_err(Func&& func) && {
        return is_ok ? std::move(*this) : func(std::move(err_val));
    }

    /// low level utility to get the current value of a Result as the raw type it is
    void* _get_value() {
        return is_ok ? static_cast<void*>(&ok_val) : static_cast<void*>(&err_val);
    }

    explicit operator bool() const noexcept { return is_ok; }
};

// experimental partial application implementation, since it is so big, it might be moved to a
// functional package if finished and in a working state

struct qk_placeholder {};
inline constexpr qk_placeholder _{};

template <class T>
struct is_qk_placeholder : std::false_type {};

template <>
struct is_qk_placeholder<qk_placeholder> : std::true_type {};

template <class... Args>
constexpr int count_placeholders() {
    return (0 + ... + (is_qk_placeholder<std::decay_t<Args>>::value ? 1 : 0));
}

template <size_t I, class... Args>
constexpr size_t count_placeholders_before() {
    size_t count = 0;
    size_t idx = 0;
    ((idx++ < I && is_qk_placeholder<std::decay_t<Args>>::value ? ++count : count), ...);
    return count;
}

template <size_t I, typename Tuple, typename NextTuple, typename... OrigArgs>
decltype(auto) get_arg(const Tuple& bound, NextTuple&& next_tuple) {
    using BoundType = std::tuple_element_t<I, std::remove_cvref_t<Tuple>>;
    if constexpr (is_qk_placeholder<std::decay_t<BoundType>>::value) {
        constexpr size_t placeholder_idx = count_placeholders_before<I, OrigArgs...>();
        return std::get<placeholder_idx>(std::forward<NextTuple>(next_tuple));
    } else {
        return std::get<I>(bound);
    }
}

template <typename Func, typename BoundTuple, typename NextTuple, typename... OrigArgs>
struct result_type_impl {
    template <size_t... Is>
    static auto deduce(std::index_sequence<Is...>) -> decltype(std::invoke(
        std::declval<Func>(), get_arg<Is, BoundTuple, NextTuple, OrigArgs...>(
                                  std::declval<BoundTuple>(), std::declval<NextTuple>()
                              )...
    ));

    using type = decltype(deduce(std::make_index_sequence<sizeof...(OrigArgs)>{}));
};

template <typename Func, typename BoundTuple, typename NextTuple, typename... OrigArgs>
using result_type = typename result_type_impl<Func, BoundTuple, NextTuple, OrigArgs...>::type;

template <typename T>
struct function_traits;

template <typename R, typename... A>
struct function_traits<R(A...)> {
    using return_type = R;
    using args_tuple = std::tuple<A...>;
};

template <typename R, typename... A>
struct function_traits<R (*)(A...)> : function_traits<R(A...)> {};

template <typename R, typename C, typename... A>
struct function_traits<R (C::*)(A...)> : function_traits<R(A...)> {};

template <typename R, typename C, typename... A>
struct function_traits<R (C::*)(A...) const> : function_traits<R(A...)> {};

template <typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};

template <typename Seq, size_t N>
struct append_index;

template <size_t... Is, size_t N>
struct append_index<std::index_sequence<Is...>, N> {
    using type = std::index_sequence<Is..., N>;
};

template <size_t I, typename... Rest>
struct placeholder_indices_helper;

template <size_t I>
struct placeholder_indices_helper<I> {
    using type = std::index_sequence<>;
};

template <size_t I, typename Head, typename... Tail>
struct placeholder_indices_helper<I, Head, Tail...> {
    using type = typename placeholder_indices_helper<I + 1, Tail...>::type;
};

template <size_t I, typename... Tail>
struct placeholder_indices_helper<I, qk_placeholder, Tail...> {
    using recursive = typename placeholder_indices_helper<I + 1, Tail...>::type;
    using type = typename append_index<recursive, I>::type;
};

template <typename OrigArgsTuple, typename PlaceholderPosSeq>
struct next_args_tuple_builder;

template <typename OrigArgsTuple, size_t... Pos>
struct next_args_tuple_builder<OrigArgsTuple, std::index_sequence<Pos...>> {
    using type = std::tuple<std::tuple_element_t<Pos, OrigArgsTuple>...>;
};

template <
    size_t Arity, typename NextArgsTuple, typename F, typename B, typename OAT, typename... OArgs>
struct partial_applier_base;

template <typename... NextTs, typename F, typename B, typename OAT, typename... OArgs>
struct partial_applier_base<sizeof...(NextTs), std::tuple<NextTs...>, F, B, OAT, OArgs...> {
    F f;
    B bound;

    partial_applier_base(F&& func, B&& bnd)
        : f(std::forward<F>(func)), bound(std::forward<B>(bnd)) {}

    using next_tuple_type = std::tuple<NextTs&&...>;
    using result_t = result_type<F, B, next_tuple_type, OArgs...>;

    result_t operator()(NextTs&&... next_args) {
        static_assert(
            sizeof...(NextTs) == count_placeholders<OArgs...>(), "Internal arity mismatch"
        );

        auto next_tuple = std::forward_as_tuple(std::forward<NextTs>(next_args)...);

        return [&]<size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
            return std::invoke(
                this->f, get_arg<Is, B, decltype(next_tuple), OArgs...>(
                             this->bound, std::forward<decltype(next_tuple)>(next_tuple)
                         )...
            );
        }(std::make_index_sequence<sizeof...(OArgs)>{});
    }
};

template <typename F, typename B, typename NextArgsTuple, typename OAT, typename... OArgs>
struct partial_applier
    : partial_applier_base<std::tuple_size_v<NextArgsTuple>, NextArgsTuple, F, B, OAT, OArgs...> {
    using base =
        partial_applier_base<std::tuple_size_v<NextArgsTuple>, NextArgsTuple, F, B, OAT, OArgs...>;
    using base::base;
};

template <typename Func, typename... Args>
auto apply(Func&& func, Args&&... args) {
    auto bound = std::make_tuple(std::forward<Args>(args)...);

    using F = std::decay_t<Func>;
    using B = decltype(bound);
    using func_traits = function_traits<F>;
    using orig_args_tuple = typename func_traits::args_tuple;

    static_assert(
        std::tuple_size_v<orig_args_tuple> == sizeof...(Args),
        "Number of arguments must match function arity."
    );

    using placeholder_pos_seq = typename placeholder_indices_helper<0, std::decay_t<Args>...>::type;
    using next_args_tuple =
        typename next_args_tuple_builder<orig_args_tuple, placeholder_pos_seq>::type;

    return partial_applier<F, B, next_args_tuple, orig_args_tuple, std::decay_t<Args>...>{
        std::forward<Func>(func), std::move(bound)
    };
}

// second working prototype
// template <typename Func, typename... Args>
// auto apply(Func&& func, Args&&... args) {
//     constexpr int arity = count_placeholders<Args...>();
//     auto bound = std::make_tuple(std::forward<Args>(args)...);
//
//     return [f = std::forward<Func>(func), bound = std::move(bound)]<typename... NextArgs>(
//                NextArgs&&... next_args
//            ) mutable -> result_type<Func, decltype(bound), std::tuple<NextArgs&&...>, Args...> {
//         static_assert(
//             sizeof...(NextArgs) == arity, "Incorrect number of arguments for placeholders"
//         );
//
//         auto next_tuple = std::tuple<NextArgs&&...>(std::forward<NextArgs>(next_args)...);
//
//         return [&]<size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
//             return std::invoke(
//                 f, get_arg<Is, decltype(bound), decltype(next_tuple), Args...>(
//                        bound, std::forward<decltype(next_tuple)>(next_tuple)
//                    )...
//             );
//         }(std::make_index_sequence<sizeof...(Args)>{});
//     };
// }

// first working prototype
// template <typename Func, typename... Args>
// auto apply(Func&& func, Args&&... args) {
//     constexpr int arity = count_placeholders<Args...>();
//     auto bound = std::make_tuple(std::forward<Args>(args)...);
//
//     return [f = std::forward<Func>(func),
//             bound =
//                 std::move(bound)]<typename... NextArgs>(NextArgs&&... next_args) ->
//                 decltype(auto) {
//         static_assert(
//             sizeof...(NextArgs) == arity, "Incorrect number of arguments for placeholders"
//         );
//
//         auto next_tuple = std::tuple<NextArgs&&...>(std::forward<NextArgs>(next_args)...);
//
//         return [&]<size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
//             return std::invoke(f, [&]() -> decltype(auto) {
//                 using BoundType = std::tuple_element_t<Is, std::remove_cvref_t<decltype(bound)>>;
//                 if constexpr (is_qk_placeholder<std::decay_t<BoundType>>::value) {
//                     constexpr size_t placeholder_idx = count_placeholders_before<Is, Args...>();
//                     return std::get<placeholder_idx>(std::move(next_tuple));
//                 } else {
//                     return std::get<Is>(bound);
//                 }
//             }()...);
//         }(std::make_index_sequence<sizeof...(Args)>{});
//     };
// }

}  // namespace qk::utils

#endif

#endif  // UTILS_H
