#ifndef INCREMENTER_H
#define INCREMENTER_H

#ifdef QK_UTILS

#include <cassert>
#include <charconv>
#include <chrono>
#include <concepts>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace qk::utils {

template <typename T>
concept parsable_arithmetic = std::is_arithmetic_v<T> || requires(std::string_view sv, T& out) {
    { parse_number(sv, out) } -> std::convertible_to<bool>;
};

template <typename T>
    requires std::is_arithmetic_v<T>
constexpr bool parse_number(std::string_view sv, T& out) noexcept {
    if (sv.empty()) return false;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), out);
    return ec == std::errc();
}

template <parsable_arithmetic Num = double, typename Duration = std::chrono::nanoseconds>
struct Rate {
    Num amount{};
    Duration interval{};

    constexpr Rate() = default;
    constexpr Rate(Num a, Duration d) : amount(a), interval(d) {}

    constexpr double per_second() const noexcept {
        using namespace std::chrono;
        assert(interval.count() != 0 && "Interval cannot be zero");
        return static_cast<double>(amount) / duration_cast<seconds>(interval).count();
    }
};

enum class TimeUnit { Second, Millisecond, Minute, Hour };

constexpr std::optional<TimeUnit> parse_unit(std::string_view sv) noexcept {
    if (sv == "s") return TimeUnit::Second;
    if (sv == "ms") return TimeUnit::Millisecond;
    if (sv == "m") return TimeUnit::Minute;
    if (sv == "h") return TimeUnit::Hour;
    return std::nullopt;
}

constexpr std::chrono::nanoseconds unit_to_duration(TimeUnit u, double count) noexcept {
    using namespace std::chrono;
    switch (u) {
        case TimeUnit::Second:
            return duration_cast<nanoseconds>(duration<double>(count));
        case TimeUnit::Millisecond:
            return duration_cast<nanoseconds>(duration<double, std::milli>(count));
        case TimeUnit::Minute:
            return duration_cast<nanoseconds>(duration<double, std::ratio<60>>(count));
        case TimeUnit::Hour:
            return duration_cast<nanoseconds>(duration<double, std::ratio<3600>>(count));
    }
    return nanoseconds{0};
}

template <parsable_arithmetic Num = double>
constexpr bool parse_rate(std::string_view str, Rate<Num>& out) noexcept {
    auto slash = str.find('/');
    if (slash == std::string_view::npos) return false;

    std::string_view lhs = str.substr(0, slash);
    std::string_view rhs = str.substr(slash + 1);

    Num amount{};
    if (!lhs.empty()) {
        if (!parse_number(lhs, amount)) return false;
    } else {
        amount = static_cast<Num>(1);
    }

    Num denom_value = static_cast<Num>(1);
    std::string_view unit_part = rhs;

    size_t i = 0;
    while (i < rhs.size() && (std::isdigit(rhs[i]) || rhs[i] == '.' || rhs[i] == '-'))
        ++i;

    if (i > 0) {
        std::string_view num_part = rhs.substr(0, i);
        if (!parse_number(num_part, denom_value)) return false;
        unit_part = rhs.substr(i);
    }

    auto unit_opt = parse_unit(unit_part);
    if (!unit_opt.has_value()) return false;

    out = Rate<Num>{amount, unit_to_duration(*unit_opt, static_cast<double>(denom_value))};
    return true;
}

constexpr Rate<> operator""_rate(const char* str, size_t len) noexcept {
    Rate r{};
    parse_rate(std::string_view(str, len), r);
    return r;
}

template <typename Num, typename Duration>
std::string to_string(const Rate<Num, Duration>& r) {
    using namespace std::chrono;
    auto ns = duration_cast<nanoseconds>(r.interval).count();

    std::string unit = "ns";
    double val = static_cast<double>(ns);

    if (ns % 1'000'000'000 == 0) {
        unit = "s";
        val /= 1'000'000'000;
    } else if (ns % 1'000'000 == 0) {
        unit = "ms";
        val /= 1'000'000;
    } else if (ns % 60'000'000'000 == 0) {
        unit = "m";
        val /= 60'000'000'000;
    }

    return std::to_string(r.amount) + "/" + std::to_string(val) + unit;
}

struct incrementer_storage;
using update_fn = void (*)(incrementer_storage*, std::chrono::nanoseconds);
using set_rate_fn = void (*)(incrementer_storage*, std::string_view);
using multiply_rate_fn = void (*)(incrementer_storage*, double);
using divide_rate_fn = void (*)(incrementer_storage*, double);

extern std::vector<incrementer_storage> incrementers;

struct incrementer_storage {
    void* target_ptr;
    void* rate_ptr;
    void* accumulator_ptr;

    update_fn update;
    set_rate_fn set_rate;
    multiply_rate_fn multiply_rate;
    divide_rate_fn divide_rate;

    void (*destroy)(incrementer_storage*);
};

template <typename T>
void update_impl(incrementer_storage* storage, std::chrono::nanoseconds delta) {
    T* val = static_cast<T*>(storage->target_ptr);
    auto* rate = static_cast<Rate<T>*>(storage->rate_ptr);
    T* accumulator = static_cast<T*>(storage->accumulator_ptr);
    if (rate->interval.count() == 0) return;

    double ratio = static_cast<double>(delta.count()) / static_cast<double>(rate->interval.count());
    double exact_increment = static_cast<double>(rate->amount) * ratio;
    if constexpr (std::is_integral_v<T>) {
        *accumulator += static_cast<T>(exact_increment);
        double fractional = exact_increment - static_cast<double>(static_cast<T>(exact_increment));
        *accumulator += static_cast<T>(fractional * 1000) / static_cast<T>(1000);

        T to_add = static_cast<T>(*accumulator);
        *accumulator -= to_add;
        *val += to_add;
    } else {
        *val += static_cast<T>(exact_increment);
    }
}

template <typename T>
void set_rate_impl(incrementer_storage* storage, std::string_view rate_str) {
    auto* rate = static_cast<Rate<T>*>(storage->rate_ptr);
    Rate<T> new_rate;
    if (parse_rate(rate_str, new_rate)) {
        *rate = new_rate;
    }
}

template <typename T>
void multiply_rate_impl(incrementer_storage* storage, double factor) {
    auto* rate = static_cast<Rate<T>*>(storage->rate_ptr);
    rate->amount *= static_cast<T>(factor);
}

template <typename T>
void divide_rate_impl(incrementer_storage* storage, double divisor) {
    auto* rate = static_cast<Rate<T>*>(storage->rate_ptr);
    if (divisor != 0.0) {
        rate->amount /= static_cast<T>(divisor);
    }
}

template <typename T>
void destroy_impl(incrementer_storage* storage) {
    delete static_cast<Rate<T>*>(storage->rate_ptr);
    delete static_cast<T*>(storage->accumulator_ptr);
}

struct incrementer_handle {
    size_t index;

    bool operator==(const incrementer_handle& other) const { return index == other.index; }

    void rate(std::string_view sv) const;
};

template <typename T>
incrementer_handle register_incrementer(T* target, Rate<T> rate = {}) {
    incrementer_storage storage{};
    storage.target_ptr = target;
    storage.rate_ptr = new Rate<T>(rate);
    storage.accumulator_ptr = new T{};

    storage.update = &update_impl<T>;
    storage.set_rate = &set_rate_impl<T>;
    storage.multiply_rate = &multiply_rate_impl<T>;
    storage.divide_rate = &divide_rate_impl<T>;
    storage.destroy = &destroy_impl<T>;

    incrementers.push_back(storage);
    return incrementer_handle{incrementers.size() - 1};
}

inline void update_all(std::chrono::nanoseconds delta) {
    for (auto& inc : incrementers) {
        inc.update(&inc, delta);
    }
}

inline void set_rate(incrementer_handle handle, std::string_view rate_str) {
    if (handle.index < incrementers.size()) {
        incrementers[handle.index].set_rate(&incrementers[handle.index], rate_str);
    }
}

inline void incrementer_handle::rate(std::string_view sv) const { set_rate(*this, sv); }

inline void multiply_rate(const incrementer_handle& handle, double factor) {
    if (handle.index < incrementers.size()) {
        incrementers[handle.index].multiply_rate(&incrementers[handle.index], factor);
    }
}

inline incrementer_handle operator*(const incrementer_handle& handle, double factor) {
    multiply_rate(handle, factor);
    return handle;
}

inline void divide_rate(const incrementer_handle& handle, double divisor) {
    if (handle.index < incrementers.size()) {
        incrementers[handle.index].divide_rate(&incrementers[handle.index], divisor);
    }
}

inline incrementer_handle operator/(const incrementer_handle& handle, double factor) {
    divide_rate(handle, factor);
    return handle;
}

inline void cleanup_incrementers() {
    for (auto& inc : incrementers) {
        if (inc.destroy) {
            inc.destroy(&inc);
        }
    }
    incrementers.clear();
}

}  // namespace qk::utils

#endif

#endif  // INCREMENTER_H
