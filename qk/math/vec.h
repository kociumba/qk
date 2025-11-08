#ifndef QK_VEC_H
#define QK_VEC_H

#ifdef QK_MATH

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <string>
#include <type_traits>

namespace qk::math {

template <typename Derived, size_t N, typename T = float>
    requires std::is_arithmetic_v<T>
struct vec_base {
    Derived& self() { return static_cast<Derived&>(*this); }
    const Derived& self() const { return static_cast<const Derived&>(*this); }

    [[nodiscard]] static constexpr auto size() { return N; }

    T& operator[](size_t i) { return self().data[i]; }
    const T& operator[](size_t i) const { return self().data[i]; }

    Derived& operator+=(const Derived& other) {
        for (size_t i = 0; i < N; ++i) {
            self()[i] += other[i];
        }
        return self();
    }

    Derived operator+(const Derived& other) const {
        Derived result = self();
        result += other;
        return result;
    }

    Derived& operator-=(const Derived& other) {
        for (size_t i = 0; i < N; ++i) {
            self()[i] -= other[i];
        }
        return self();
    }

    Derived operator-(const Derived& other) const {
        Derived result = self();
        result -= other;
        return result;
    }

    Derived& operator*=(T scalar) {
        for (size_t i = 0; i < N; ++i) {
            self()[i] *= scalar;
        }
        return self();
    }

    Derived operator*(T scalar) const {
        Derived result = self();
        result *= scalar;
        return result;
    }

    Derived& operator/=(T scalar) {
        for (size_t i = 0; i < N; ++i) {
            self()[i] /= scalar;
        }
        return self();
    }

    Derived operator/(T scalar) const {
        Derived result = self();
        result /= scalar;
        return result;
    }

    Derived operator-() const {
        Derived result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = -self()[i];
        }
        return result;
    }

    T dot(const Derived& other) const {
        T result = 0;
        for (size_t i = 0; i < N; ++i) {
            result += self()[i] * other[i];
        }
        return result;
    }

    [[nodiscard]] T length_squared() const { return dot(self()); }

    [[nodiscard]] T length() const { return std::sqrt(length_squared()); }

    [[nodiscard]] Derived normalized() const {
        T len = length();
        return self() / len;
    }

    Derived& normalize() { return *this = normalized(); }

    bool operator==(const Derived& other) const {
        for (size_t i = 0; i < N; ++i) {
            if (self()[i] != other[i]) return false;
        }
        return true;
    }

    bool operator!=(const Derived& other) const { return !(*this == other); }

    [[nodiscard]] T distance_squared(const Derived& other) const {
        return (self() - other).length_squared();
    }

    [[nodiscard]] T distance(const Derived& other) const { return (self() - other).length(); }

    [[nodiscard]] static Derived lerp(const Derived& a, const Derived& b, T t) {
        return a + (b - a) * t;
    }

    Derived multiply(const Derived& other) const {
        Derived result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = self()[i] * other[i];
        }
        return result;
    }

    Derived min(const Derived& other) const {
        Derived result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = std::min(self()[i], other[i]);
        }
        return result;
    }

    Derived max(const Derived& other) const {
        Derived result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = std::max(self()[i], other[i]);
        }
        return result;
    }

    Derived clamp(const Derived& min_v, const Derived& max_v) const {
        Derived result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = std::clamp(self()[i], min_v[i], max_v[i]);
        }
        return result;
    }

    Derived abs() const {
        Derived result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = std::abs(self()[i]);
        }
        return result;
    }

    Derived project(const Derived& onto) const {
        T scale = dot(onto) / onto.dot(onto);
        return onto * scale;
    }

    Derived reject(const Derived& from) const { return self() - project(from); }

    Derived reflect(const Derived& normal) const { return self() - normal * (2 * dot(normal)); }

    [[nodiscard]] T angle(const Derived& other) const {
        T cos_angle = dot(other) / (length() * other.length());
        return std::acos(std::clamp(cos_angle, T(-1), T(1)));
    }

    bool approx_equal(const Derived& other, T epsilon = T(1e-6)) const {
        for (size_t i = 0; i < N; ++i) {
            if (std::abs(self()[i] - other[i]) > epsilon) return false;
        }
        return true;
    }

    bool is_normalized(T epsilon = T(1e-6)) const {
        return std::abs(length_squared() - T(1)) < epsilon;
    }

    bool is_zero(T epsilon = T(1e-6)) const {
        for (size_t i = 0; i < N; ++i) {
            if (std::abs(self()[i]) > epsilon) return false;
        }
        return true;
    }

    [[nodiscard]] Derived safe_normalized(T epsilon = T(1e-8)) const {
        T len_sq = length_squared();
        if (len_sq < epsilon * epsilon) {
            return Derived{};
        }
        return self() / std::sqrt(len_sq);
    }

    [[nodiscard]] Derived with_length(T new_length) const {
        T len = length();
        if (len < T(1e-8)) return Derived{};
        return self() * (new_length / len);
    }

    [[nodiscard]] Derived constrain_length(T max_length) const {
        T len_sq = length_squared();
        if (len_sq <= max_length * max_length) {
            return self();
        }
        return self() * (max_length / std::sqrt(len_sq));
    }

    [[nodiscard]] T manhattan_distance(const Derived& other) const {
        T result = 0;
        for (size_t i = 0; i < N; ++i) {
            result += std::abs(self()[i] - other[i]);
        }
        return result;
    }

    [[nodiscard]] static Derived zero() { return Derived{}; }

    [[nodiscard]] static Derived one() {
        Derived result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = T(1);
        }
        return result;
    }
};

template <typename Derived, size_t N, typename T>
    requires std::is_arithmetic_v<T>
Derived operator*(T scalar, const vec_base<Derived, N, T>& v) {
    return v.self() * scalar;
}

template <size_t N, typename T = float>
    requires std::is_arithmetic_v<T>
struct vec : vec_base<vec<N, T>, N, T> {
    T data[N];

    vec() : data{} {}
    template <typename... Args>
    vec(Args... args) : data{static_cast<T>(args)...} {
        static_assert(sizeof...(Args) == N, "Incorrect number of arguments");
    }
};

template <size_t N>
using ivec = vec<N, int>;

template <typename T = float>
    requires std::is_arithmetic_v<T>
struct vec2 : vec_base<vec2<T>, 2, T> {
    union {
        T data[2];
        struct {
            T x, y;
        };
    };

    vec2() : data{} {}

    template <typename... Args>
    vec2(Args... args) : data{static_cast<T>(args)...} {
        static_assert(sizeof...(Args) == 2, "Incorrect number of arguments");
    }

    vec2 perpendicular() const { return vec2(-y, x); }

    vec2 rotate(T angle) const {
        T c = std::cos(angle);
        T s = std::sin(angle);
        return vec2(x * c - y * s, x * s + y * c);
    }

    static vec2 from_polar(T angle, T length = T(1)) {
        return vec2(std::cos(angle) * length, std::sin(angle) * length);
    }

    T to_angle() const { return std::atan2(y, x); }
};

using ivec2 = vec2<int>;

template <typename T = float>
    requires std::is_arithmetic_v<T>
struct vec3 : vec_base<vec3<T>, 3, T> {
    union {
        T data[3];
        struct {
            T x, y, z;
        };
    };

    vec3() : data{} {}

    template <typename... Args>
    vec3(Args... args) : data{static_cast<T>(args)...} {
        static_assert(sizeof...(Args) == 3, "Incorrect number of arguments");
    }

    vec3 cross(const vec3& other) const {
        return vec3(
            this->y * other.z - this->z * other.y, this->z * other.x - this->x * other.z,
            this->x * other.y - this->y * other.x
        );
    }

    vec3 rotate(const vec3& axis, T angle) const {
        T c = std::cos(angle);
        T s = std::sin(angle);
        vec3 k = axis;
        return *this * c + k.cross(*this) * s + k * (k.dot(*this) * (T(1) - c));
    }

    std::pair<vec3, vec3> orthonormal_basis() const {
        vec3 n = this->normalized();
        vec3 t;

        if (std::abs(n.x) < std::abs(n.y) && std::abs(n.x) < std::abs(n.z)) {
            t = vec3(T(1), T(0), T(0));
        } else if (std::abs(n.y) < std::abs(n.z)) {
            t = vec3(T(0), T(1), T(0));
        } else {
            t = vec3(T(0), T(0), T(1));
        }

        vec3 tangent = t.reject(n).normalized();
        vec3 bitangent = n.cross(tangent);

        return {tangent, bitangent};
    }

    static vec3 unit_x() { return vec3(T(1), T(0), T(0)); }
    static vec3 unit_y() { return vec3(T(0), T(1), T(0)); }
    static vec3 unit_z() { return vec3(T(0), T(0), T(1)); }
    static vec3 up() { return vec3(T(0), T(1), T(0)); }
    static vec3 down() { return vec3(T(0), T(-1), T(0)); }
    static vec3 right() { return vec3(T(1), T(0), T(0)); }
    static vec3 left() { return vec3(T(-1), T(0), T(0)); }
    static vec3 forward() { return vec3(T(0), T(0), T(-1)); }
    static vec3 back() { return vec3(T(0), T(0), T(1)); }
};

using ivec3 = vec3<int>;

template <typename T = float>
    requires std::is_arithmetic_v<T>
struct vec4 : vec_base<vec4<T>, 4, T> {
    union {
        T data[4];
        struct {
            T x, y, z, w;
        };
    };

    vec4() : data{} {}

    template <typename... Args>
    vec4(Args... args) : data{static_cast<T>(args)...} {
        static_assert(sizeof...(Args) == 4, "Incorrect number of arguments");
    }
};

using ivec4 = vec4<int>;

struct color {
    float r, g, b, a;

    constexpr color() : r(0), g(0), b(0), a(1) {}
    constexpr color(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}

    constexpr color(uint32_t hex) {
        if (hex <= 0xFFFFFF) {
            r = ((hex >> 16) & 0xFF) / 255.0f;
            g = ((hex >> 8) & 0xFF) / 255.0f;
            b = (hex & 0xFF) / 255.0f;
            a = 1.0f;
        } else {
            r = ((hex >> 24) & 0xFF) / 255.0f;
            g = ((hex >> 16) & 0xFF) / 255.0f;
            b = ((hex >> 8) & 0xFF) / 255.0f;
            a = (hex & 0xFF) / 255.0f;
        }
    }

    constexpr static color from_rgb(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) {
        return color(red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f);
    }

    template <typename T = float>
    constexpr operator vec4<T>() const {
        return vec4<T>(static_cast<T>(r), static_cast<T>(g), static_cast<T>(b), static_cast<T>(a));
    }
};

constexpr uint8_t hex_char(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(10 + (c - 'a'));
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(10 + (c - 'A'));
    return 0;
}

constexpr uint32_t parse_hex(std::string_view s) {
    uint32_t value = 0;
    for (char c : s)
        value = value << 4 | hex_char(c);
    return value;
}

constexpr color operator""_col(const char* str, size_t len) {
    std::string_view s(str, len);
    if (!s.empty() && s.front() == '#') s.remove_prefix(1);

    assert(s.size() == 6 || s.size() == 8 && "Invalid color literal length");

    uint32_t hex = parse_hex(s);
    return {hex};
}

}  // namespace qk::math

#endif

#endif  // QK_VEC_H
