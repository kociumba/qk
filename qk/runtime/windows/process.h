#ifndef PROCESS_H
#define PROCESS_H

#ifdef QK_RUNTIME_UTILS

#include <algorithm>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../api.h"
#include "memory.h"

namespace qk::runtime::proc {

using namespace qk::runtime::mem;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <TlHelp32.h>

struct QK_API Image {
    std::uintptr_t base;
    size_t size;
    byte_vec bytes;

    bool is64 = false;
};

struct QK_API Process {
    HANDLE process;
    HWND hwnd = nullptr;
    DWORD pid;

    bool is64 = false;

    std::unordered_map<std::string, std::unique_ptr<Image>> images;
    // dumper, hooks, shared_memory, whatever else we need

    // delete copy to allow for unique_ptr in map
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&&) = default;
    Process& operator=(Process&&) = default;
};

QK_API inline std::wstring to_wstring(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size_needed == 0) {
        return std::wstring();
    }

    std::wstring wstr(size_needed, 0);

    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);

    return wstr;
}

QK_API bool refresh_image_map(Process* proc);
QK_API bool read_image(byte_vec* dest, const std::string& image_name, Process* proc);
QK_API bool setup_process(DWORD pid, Process* proc);
QK_API bool setup_process(std::string ident, const bool& is_proc_name, Process* proc);
QK_API LPVOID alloc_page_in_proc(Process* proc, DWORD prot, size_t size = 4096);
QK_API bool inject_lib(const std::string& path, Process* proc);

// should search in the images
QK_API std::uintptr_t find_pattern(const byte_vec& pattern, const bool& relative, Process* proc);
QK_API std::uintptr_t find_pattern(const byte_vec& pattern, const bool& relative, Image* image);
QK_API std::uintptr_t find_pattern(
    const byte_vec& pattern, const bool& relative, const std::string& image_name, Process* proc
);

}  // namespace qk::runtime::proc

#endif

#endif  // PROCESS_H
