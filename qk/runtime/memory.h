#ifndef MEMORY_WINDOWS_H
#define MEMORY_WINDOWS_H

#ifdef QK_RUNTIME_UTILS

#include <cstddef>
#include <locale>
#include <string_view>
#include <vector>
#include "../api.h"

namespace qk::runtime::mem {

#ifndef WILDCARD_BYTE
#define WILDCARD_BYTE 0xCC
#endif

using byte_vec = std::vector<std::byte>;

struct QK_API Image {
    std::uintptr_t base;
    size_t size;
    byte_vec bytes;

    bool is64 = false;
};

constexpr int hex_to_nibble(char c) noexcept {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

QK_API bool parse_signature(std::string_view sig, byte_vec& out);

QK_API inline byte_vec operator"" _sig(const char* str, size_t len) {
    byte_vec b;
    parse_signature(std::string_view(str, len), b);
    return b;
}

}  // namespace qk::runtime::mem

#endif

#endif  // MEMORY_WINDOWS_H
