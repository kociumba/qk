#ifndef UTILS_H
#define UTILS_H

#ifdef QK_UTILS

#include <algorithm>
#include <concepts>
#include <functional>
#include <optional>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>

#include "../api.h"

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

// generic stream type, simplifies concatenation and repeated operations on arrays
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
        if constexpr (std::is_rvalue_reference_v<R&&>) {
            std::ranges::move(range, std::back_inserter(data));
        } else {
            std::ranges::copy(range, std::back_inserter(data));
        }
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

    void reserve(size_t n) { data.reserve(n); }
    void clear() noexcept { data.clear(); }

    auto begin() { return data.begin(); }
    auto begin() const { return data.begin(); }
    auto end() { return data.end(); }
    auto end() const { return data.end(); }
};

template <typename T>
stream(T) -> stream<T>;

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
