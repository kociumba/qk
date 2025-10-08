#ifndef EMBEDDING_H
#define EMBEDDING_H

#ifdef QK_EMBEDDING

#include <zlib.h>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include "../api.h"

/// simple convenience macro to define the extern for a qk embedded data begin, file_name should be
/// the name of the embedded file with all ascii incompatible characters replaced with _
#define QK_GET_EMBED_DATA(file_name) extern const unsigned char file_name##_data[]

/// simple convenience macro to define the extern for a qk embedded data end, file_name should be
/// the name of the embedded file with all ascii incompatible characters replaced with _
#define QK_GET_EMBED_END(file_name) extern const unsigned char file_name##_end[]

/// simple convenience macro to define the extern for a qk embedded data size, file_name should be
/// the name of the embedded file with all ascii incompatible characters replaced with _
#define QK_GET_EMBED_SIZE(file_name) extern const uint64_t file_name##_size

namespace qk::embed {

#ifdef _WIN32
static std::string object_ext = ".obj";
#else
static std::string object_ext = ".o";
#endif

const std::string comp_time_nasm_path = QK_NASM;

inline std::string nasm_path() {
    std::string command;
#ifdef _WIN32
    command = "where nasm";
#else
    command = "which nasm";
#endif

    char buffer[256];
    std::string result;

    FILE* pipe = nullptr;
#ifdef _WIN32
    pipe = _popen(command.c_str(), "r");
#else
    pipe = popen(command.c_str(), "r");
#endif

    if (!pipe) {
        return comp_time_nasm_path;
    }

    if (std::fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result = buffer;
    }

    int status;
#ifdef _WIN32
    status = _pclose(pipe);
#else
    status = pclose(pipe);
#endif

    if (status == 0 && !result.empty()) {
        size_t last = result.find_last_not_of("\n\r");
        if (last != std::string::npos) {
            result.erase(last + 1);
        }
        return result;
    }

    return comp_time_nasm_path;
}

enum class Target { ELF, PE, MACH_O };
enum class Arch {
    x64,
    x32,
};

static std::unordered_map<Target, std::string_view> format_map = {
    {Target::ELF, "elf"},
    {Target::PE, "win"},
    {Target::MACH_O, "macho"},
};

static std::unordered_map<Arch, std::string_view> arch_map = {
    {Arch::x64, "64"},
    {Arch::x32, "32"},
};

#ifdef _WIN32

constexpr auto default_target = Target::PE;
#ifdef _WIN64
constexpr auto default_arch = Arch::x64;
#else
constexpr auto default_arch = Arch::x32;
#endif

#elif __linux__

constexpr auto default_target = Target::ELF;
#if defined(__x86_64__)
constexpr auto default_arch = Arch::x64;
#elif defined(__i386__)
constexpr auto default_arch = Arch::x32;
#elif defined(__aarch64__)
constexpr auto default_arch = Arch::x64;
#elif defined(__arm__)
constexpr auto default_arch = Arch::x32;

#else
constexpr auto default_arch = Arch::x64;
#endif

#elif __APPLE__
constexpr auto default_target = Target::MACH_O;
#if defined(__x86_64__)
constexpr auto default_arch = Arch::x64;
#elif defined(__i386__)
constexpr auto default_arch = Arch::x32;
#elif defined(__aarch64__)
constexpr auto default_arch = Arch::x64;
#else

constexpr auto default_arch = Arch::x64;
#endif

#else

constexpr auto default_target = Target::ELF;
constexpr auto default_arch = Arch::x64;
#endif

struct QK_API Binary {
    Target format;
    Arch arch;
    std::vector<std::string> symbols;
    std::vector<std::byte> data;

    std::string nasm_path;
    std::string asm_path;

    bool render();
    bool assemble();
};

QK_API bool compress_object(Binary* bin, int level = Z_DEFAULT_COMPRESSION);
QK_API std::vector<std::byte> decompress_data(const unsigned char* data, uint64_t size);

QK_API Binary make_object(
    const std::string& name, Target format = default_target, Arch arch = default_arch,
    const std::string& nasm = nasm_path()
);

[[deprecated("this api can produce corrupted arm binaries")]] QK_API bool patch_macho_arm64(
    const std::filesystem::path& path
);

}  // namespace qk::embed

#endif

#endif  // EMBEDDING_H
