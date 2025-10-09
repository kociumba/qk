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

#ifdef __linux__
#define RUNTIME_DEPRECATION                                                                        \
    [[deprecated(                                                                                  \
        "runtime discovery is currently unsupported on linux if the executable is not built with " \
        "'-rdynamic'"                                                                              \
    )]]
#else
#define RUNTIME_DEPRECATION
#endif

/// defines utilities for embedding and retrieving embedded data in binaries
namespace qk::embed {

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

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

QK_API inline void sanitize_symbol(std::string& name) {
    for (auto& c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c))) {
            c = '_';
        }
    }
}

/// utility to get the symbol base used by the embedding system from a file name
QK_API inline std::string filename_to_symbol(const std::string& filename) {
    std::filesystem::path filepath(filename);
    std::string base_name = filepath.stem().string();
    sanitize_symbol(base_name);
    return base_name;
}

/// used in place of static compile time variables when using the runtime symbol resolution
struct QK_API Resource {
    const unsigned char* data = nullptr;
    const unsigned char* data_end = nullptr;
    uint64_t size = 0;

    [[nodiscard]] bool is_valid() const {
        return data != nullptr && data_end != nullptr && size != 0;
    }
};

#ifdef __APPLE__
struct StringHash {
    size_t operator()(const std::string& str) const noexcept {
        size_t hash = 1469598103934665603ULL;
        for (char c : str) {
            hash ^= static_cast<size_t>(c);
            hash *= 0x9e3779b97f4a7c15ULL;
        }
        return hash;
    }
};
#endif

/// caches symbols when using runtime resolution to make subsequent queries for the same static data
/// faster
struct QK_API SymbolCache {
#ifdef _WIN32
    HANDLE handle;
#else
    void* handle;
#endif

#ifdef __APPLE__
    std::unordered_map<std::string, std::string, StringHash> file_to_symbol;
    std::unordered_map<std::string, Resource, StringHash> symbol_to_resource;
#else
    std::unordered_map<std::string, std::string> file_to_symbol;
    std::unordered_map<std::string, Resource> symbol_to_resource;
#endif

    void clear() {
        handle = nullptr;
        file_to_symbol.clear();
        symbol_to_resource.clear();
    }
};

extern SymbolCache default_symbol_cache;

/// sets up 'SymbolCache' with valid state
///
/// in most scenarios this does not need to be called manually
RUNTIME_DEPRECATION QK_API bool setup_cache(SymbolCache* cache = &default_symbol_cache);

/// finds a symbol at runtime in the current binary, mostly for internal use
RUNTIME_DEPRECATION QK_API void* find_symbol(
    const std::string& name, SymbolCache* cache = &default_symbol_cache
);

/// the main way to find embedded data at runtime
///
/// it expects the file name of the file that was embedded, the returned 'Resource' can be checked
/// with '.is_valid()' to see if the embedding was found
///
/// !!! CURRENTLY DEPRECATED ON LINUX DUE TO IMPLEMENTATION ISSUES !!!
RUNTIME_DEPRECATION QK_API Resource
find_resource(const std::string& filename, SymbolCache* cache = &default_symbol_cache);

/// compresses the data in a 'Binary', to prepare it for embedding
QK_API bool compress_object(Binary* bin, int level = Z_DEFAULT_COMPRESSION);

/// decompress data from an embedded object, should not be called on data that was not compressed at
/// compile time
QK_API std::vector<std::byte> decompress_data(const unsigned char* data, uint64_t size);

/// creates a 'Binary' object preloaded with data needed for assembling it into an object
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
