#ifndef TRAITS_EXTRA_H
#define TRAITS_EXTRA_H

#ifdef QK_TRAITS_EXTRA

#include <algorithm>
#include <array>
#include <concepts>
#include <deque>
#include <forward_list>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <reflect>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace qk::traits {

namespace vh {

template <class T, class = void>
struct has_std_hash : std::false_type {};

template <class T>
struct has_std_hash<
    T, std::void_t<decltype(std::declval<std::hash<T>>()(std::declval<T const&>()))>>
    : std::true_type {};

template <class T>
inline constexpr bool has_std_hash_v = has_std_hash<T>::value;

inline std::size_t combine_hash(std::size_t seed, std::size_t v) noexcept {
    return seed ^ v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

template <typename T, typename = void>
struct is_reflectable : std::false_type {};

template <typename T>
struct is_reflectable<T, std::void_t<decltype(reflect::size(std::declval<T>()))>> : std::true_type {
};

template <typename T>
inline constexpr bool is_reflectable_v = is_reflectable<T>::value;

template <typename T>
concept IsVector = requires(T t) {
    typename T::value_type;
    typename T::allocator_type;
    { t.begin() } -> std::same_as<typename T::iterator>;
    { t.end() } -> std::same_as<typename T::iterator>;
} && requires {
    requires std::same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;
};

template <typename T>
concept IsDeque = requires(T t) {
    typename T::value_type;
    typename T::allocator_type;
} && requires {
    requires std::same_as<T, std::deque<typename T::value_type, typename T::allocator_type>>;
};

template <typename T>
concept IsList = requires(T t) {
    typename T::value_type;
    typename T::allocator_type;
} && requires {
    requires std::same_as<T, std::list<typename T::value_type, typename T::allocator_type>>;
};

template <typename T>
concept IsForwardList = requires(T t) {
    typename T::value_type;
    typename T::allocator_type;
} && requires {
    requires std::same_as<T, std::forward_list<typename T::value_type, typename T::allocator_type>>;
};

template <typename T>
concept IsStdArray = requires {
    typename std::tuple_size<T>::type;
    typename T::value_type;
    requires std::tuple_size_v<T> > 0;
};

template <typename T>
concept IsSet = requires(T t) {
    typename T::key_type;
    typename T::key_compare;
    typename T::allocator_type;
} && requires {
    requires std::same_as<
        T, std::set<typename T::key_type, typename T::key_compare, typename T::allocator_type>>;
};

template <typename T>
concept IsMultiset = requires(T t) {
    typename T::key_type;
    typename T::key_compare;
    typename T::allocator_type;
} && requires {
    requires std::same_as<
        T,
        std::multiset<typename T::key_type, typename T::key_compare, typename T::allocator_type>>;
};

template <typename T>
concept IsUnorderedSet = requires(T t) {
    typename T::key_type;
    typename T::hasher;
    typename T::allocator_type;
} && requires {
    requires std::same_as<
        T, std::unordered_set<
               typename T::key_type, typename T::hasher, typename T::key_equal,
               typename T::allocator_type>>;
};

template <typename T>
concept IsUnorderedMultiset = requires(T t) {
    typename T::key_type;
    typename T::hasher;
    typename T::allocator_type;
} && requires {
    requires std::same_as<
        T, std::unordered_multiset<
               typename T::key_type, typename T::hasher, typename T::key_equal,
               typename T::allocator_type>>;
};

template <typename T>
concept IsMap = requires(T t) {
    typename T::key_type;
    typename T::mapped_type;
    typename T::key_compare;
    typename T::allocator_type;
} && requires {
    requires std::same_as<
        T, std::map<
               typename T::key_type, typename T::mapped_type, typename T::key_compare,
               typename T::allocator_type>>;
};

template <typename T>
concept IsMultimap = requires(T t) {
    typename T::key_type;
    typename T::mapped_type;
    typename T::key_compare;
    typename T::allocator_type;
} && requires {
    requires std::same_as<
        T, std::multimap<
               typename T::key_type, typename T::mapped_type, typename T::key_compare,
               typename T::allocator_type>>;
};

template <typename T>
concept IsUnorderedMap = requires(T t) {
    typename T::key_type;
    typename T::mapped_type;
    typename T::hasher;
    typename T::allocator_type;
} && requires {
    requires std::same_as<
        T, std::unordered_map<
               typename T::key_type, typename T::mapped_type, typename T::hasher,
               typename T::key_equal, typename T::allocator_type>>;
};

template <typename T>
concept IsUnorderedMultimap = requires(T t) {
    typename T::key_type;
    typename T::mapped_type;
    typename T::hasher;
    typename T::allocator_type;
} && requires {
    requires std::same_as<
        T, std::unordered_multimap<
               typename T::key_type, typename T::mapped_type, typename T::hasher,
               typename T::key_equal, typename T::allocator_type>>;
};

template <typename T>
concept IsPair = requires(T t) {
    typename T::first_type;
    typename T::second_type;
} && requires {
    requires std::same_as<T, std::pair<typename T::first_type, typename T::second_type>>;
};

template <typename T>
concept IsTuple = requires {
    typename std::tuple_size<T>::type;
    requires std::tuple_size_v<T> >= 0;
} && !IsStdArray<T>;

template <typename T>
concept IsOptional = requires(T t) {
    typename T::value_type;
    { t.has_value() } -> std::same_as<bool>;
} && requires { requires std::same_as<T, std::optional<typename T::value_type>>; };

template <typename T>
concept IsVariant = requires {
    typename std::variant_size<T>::type;
    requires std::variant_size_v<T> > 0;
};

template <class T>
std::size_t value_hash_impl(const T& obj) noexcept;

// Sequence containers
template <IsVector T>
std::size_t value_hash_impl(const T& vec) noexcept;

template <IsDeque T>
std::size_t value_hash_impl(const T& deq) noexcept;

template <IsList T>
std::size_t value_hash_impl(const T& lst) noexcept;

template <IsForwardList T>
std::size_t value_hash_impl(const T& flst) noexcept;

template <IsStdArray T>
std::size_t value_hash_impl(const T& arr) noexcept;

// Associative containers
template <IsSet T>
std::size_t value_hash_impl(const T& s) noexcept;

template <IsMultiset T>
std::size_t value_hash_impl(const T& ms) noexcept;

template <IsMap T>
std::size_t value_hash_impl(const T& m) noexcept;

template <IsMultimap T>
std::size_t value_hash_impl(const T& mm) noexcept;

// Unordered containers
template <IsUnorderedSet T>
std::size_t value_hash_impl(const T& us) noexcept;

template <IsUnorderedMultiset T>
std::size_t value_hash_impl(const T& ums) noexcept;

template <IsUnorderedMap T>
std::size_t value_hash_impl(const T& um) noexcept;

template <IsUnorderedMultimap T>
std::size_t value_hash_impl(const T& umm) noexcept;

// Utility types
template <IsPair T>
std::size_t value_hash_impl(const T& p) noexcept;

template <IsTuple T>
std::size_t value_hash_impl(const T& tup) noexcept;

template <IsOptional T>
std::size_t value_hash_impl(const T& opt) noexcept;

template <IsVariant T>
std::size_t value_hash_impl(const T& var) noexcept;

// String types
inline std::size_t value_hash_impl(const std::string& str) noexcept;
inline std::size_t value_hash_impl(std::string_view sv) noexcept;

template <IsVector T>
std::size_t value_hash_impl(const T& vec) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    for (auto const& el : vec) {
        seed = combine_hash(seed, value_hash_impl(el));
    }
    return seed;
}

template <IsDeque T>
std::size_t value_hash_impl(const T& deq) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    for (auto const& el : deq) {
        seed = combine_hash(seed, value_hash_impl(el));
    }
    return seed;
}

template <IsList T>
std::size_t value_hash_impl(const T& lst) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    for (auto const& el : lst) {
        seed = combine_hash(seed, value_hash_impl(el));
    }
    return seed;
}

template <IsForwardList T>
std::size_t value_hash_impl(const T& flst) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    for (auto const& el : flst) {
        seed = combine_hash(seed, value_hash_impl(el));
    }
    return seed;
}

template <IsStdArray T>
std::size_t value_hash_impl(const T& arr) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    for (auto const& el : arr) {
        seed = combine_hash(seed, value_hash_impl(el));
    }
    return seed;
}

inline std::size_t value_hash_impl(const std::string& str) noexcept {
    return std::hash<std::string_view>{}(str);
}

inline std::size_t value_hash_impl(std::string_view sv) noexcept {
    return std::hash<std::string_view>{}(sv);
}

template <IsSet T>
std::size_t value_hash_impl(const T& s) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    for (auto const& el : s) {
        seed = combine_hash(seed, value_hash_impl(el));
    }
    return seed;
}

template <IsMultiset T>
std::size_t value_hash_impl(const T& ms) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    for (auto const& el : ms) {
        seed = combine_hash(seed, value_hash_impl(el));
    }
    return seed;
}

template <IsMap T>
std::size_t value_hash_impl(const T& m) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    for (auto const& [k, v] : m) {
        seed = combine_hash(seed, value_hash_impl(k));
        seed = combine_hash(seed, value_hash_impl(v));
    }
    return seed;
}

template <IsMultimap T>
std::size_t value_hash_impl(const T& mm) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    for (auto const& [k, v] : mm) {
        seed = combine_hash(seed, value_hash_impl(k));
        seed = combine_hash(seed, value_hash_impl(v));
    }
    return seed;
}

// Unordered associative containers
// Note: These need special handling because iteration order is undefined
// We sort the hashes to ensure consistent results
template <IsUnorderedSet T>
std::size_t value_hash_impl(const T& us) noexcept {
    std::vector<std::size_t> hashes;
    hashes.reserve(us.size());
    for (auto const& el : us) {
        hashes.push_back(value_hash_impl(el));
    }
    std::ranges::sort(hashes);
    std::size_t seed = 1469598103934665603ULL;
    for (auto h : hashes) {
        seed = combine_hash(seed, h);
    }
    return seed;
}

template <IsUnorderedMultiset T>
std::size_t value_hash_impl(const T& ums) noexcept {
    std::vector<std::size_t> hashes;
    hashes.reserve(ums.size());
    for (auto const& el : ums) {
        hashes.push_back(value_hash_impl(el));
    }
    std::ranges::sort(hashes);
    std::size_t seed = 1469598103934665603ULL;
    for (auto h : hashes) {
        seed = combine_hash(seed, h);
    }
    return seed;
}

template <IsUnorderedMap T>
std::size_t value_hash_impl(const T& um) noexcept {
    std::vector<std::size_t> hashes;
    hashes.reserve(um.size());
    for (auto const& [k, v] : um) {
        std::size_t pair_hash = combine_hash(value_hash_impl(k), value_hash_impl(v));
        hashes.push_back(pair_hash);
    }
    std::ranges::sort(hashes);
    std::size_t seed = 1469598103934665603ULL;
    for (auto h : hashes) {
        seed = combine_hash(seed, h);
    }
    return seed;
}

template <IsUnorderedMultimap T>
std::size_t value_hash_impl(const T& umm) noexcept {
    std::vector<std::size_t> hashes;
    hashes.reserve(umm.size());
    for (auto const& [k, v] : umm) {
        std::size_t pair_hash = combine_hash(value_hash_impl(k), value_hash_impl(v));
        hashes.push_back(pair_hash);
    }
    std::ranges::sort(hashes);

    std::size_t seed = 1469598103934665603ULL;
    for (auto h : hashes) {
        seed = combine_hash(seed, h);
    }
    return seed;
}

template <IsPair T>
std::size_t value_hash_impl(const T& p) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    seed = combine_hash(seed, value_hash_impl(p.first));
    seed = combine_hash(seed, value_hash_impl(p.second));
    return seed;
}

template <IsTuple T>
std::size_t value_hash_impl(const T& tup) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    std::apply(
        [&seed](auto const&... elems) noexcept {
            ((seed = combine_hash(seed, value_hash_impl(elems))), ...);
        },
        tup
    );
    return seed;
}

template <IsOptional T>
std::size_t value_hash_impl(const T& opt) noexcept {
    if (!opt.has_value()) {
        return 0;
    }
    return combine_hash(1, value_hash_impl(*opt));
}

template <IsVariant T>
std::size_t value_hash_impl(const T& var) noexcept {
    std::size_t seed = combine_hash(1469598103934665603ULL, var.index());
    std::visit([&seed](auto const& val) { seed = combine_hash(seed, value_hash_impl(val)); }, var);
    return seed;
}

namespace detail {

template <class E>
std::enable_if_t<std::is_enum_v<E>, std::size_t> hash_enum(const E& e) noexcept {
    using U = std::underlying_type_t<E>;
    return std::hash<U>{}(static_cast<U>(e));
}

template <class T>
std::enable_if_t<std::is_pointer_v<T>, std::size_t> hash_pointer(const T& p) noexcept {
    return std::hash<std::uintptr_t>{}(reinterpret_cast<std::uintptr_t>(p));
}

template <class T>
std::enable_if_t<is_reflectable_v<T>, std::size_t> hash_reflected(const T& obj) noexcept {
    auto tup = reflect::to<std::tuple>(obj);
    std::size_t seed = 1469598103934665603ULL;
    std::apply(
        [&seed](auto const&... elems) noexcept {
            ((seed = combine_hash(seed, value_hash_impl(elems))), ...);
        },
        tup
    );
    return seed;
}

}  // namespace detail

template <class T>
std::size_t value_hash_impl(const T& obj) noexcept {
    if constexpr (std::is_enum_v<T>) {
        return detail::hash_enum(obj);
    } else if constexpr (has_std_hash_v<T>) {
        return std::hash<T>{}(obj);
    } else if constexpr (std::is_pointer_v<T>) {
        return detail::hash_pointer(obj);
    } else if constexpr (is_reflectable_v<T>) {
        return detail::hash_reflected(obj);
    } else {
        static_assert(
            sizeof(T) != sizeof(T),
            "Type is not hashable: no std::hash, not reflectable, and no specialized overload"
        );
        [[unreachable]];
    }
}

template <class... Ts>
std::size_t combine_values_hash(const Ts&... xs) noexcept {
    std::size_t seed = 1469598103934665603ULL;
    ((seed = combine_hash(seed, value_hash_impl(xs))), ...);
    return seed;
}

}  // namespace vh

/// implements basic value hashing, compared to Hashable it guarantees the same hash on the same
/// value of a type, but requires reflection and has a bigger compilation and runtime impact
template <typename T>
concept ValueHashable = requires(T& t) {
    { t.hash() } -> std::convertible_to<size_t>;
};

struct ValueHashable_base {
    template <class Self>
    [[nodiscard]] size_t hash(this Self const& self) noexcept {
        return vh::value_hash_impl(self);
    }
};

static_assert(ValueHashable<ValueHashable_base>);

}  // namespace qk::traits

#endif

#endif  // TRAITS_EXTRA_H
