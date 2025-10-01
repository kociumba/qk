#ifndef PROCESS_H
#define PROCESS_H

#ifdef QK_RUNTIME_UTILS

#include <algorithm>
#include <cassert>
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
    Process() = default;  // fuck c++
};

enum class QK_API GraphicsAPI {
    NONE,
    OPENGL,
    VULKAN,
    DIRECTX9,
    DIRECTX10,
    DIRECTX11,
    DIRECTX12,
};

struct QK_API GraphicsInfo {
    GraphicsAPI api = GraphicsAPI::NONE;
    std::uintptr_t context_addr = 0;
    std::string module_name;

    bool valid() const {
        if (this->api == GraphicsAPI::NONE || this->context_addr == 0 ||
            this->module_name.empty()) {
            return false;
        }

        return true;
    }
};

const __readonly QK_API std::unordered_map<GraphicsAPI, std::vector<std::string>> api_modules = {
    {GraphicsAPI::OPENGL,
     {"opengl32.dll", "nvoglv32.dll", "nvoglv64.dll", "ig8icd32.dll", "ig8icd64.dll"}},
    {GraphicsAPI::DIRECTX9, {"d3d9.dll"}},
    {GraphicsAPI::DIRECTX10, {"d3d10.dll"}},
    {GraphicsAPI::DIRECTX11, {"d3d11.dll"}},
    {GraphicsAPI::DIRECTX12, {"d3d12.dll", "dxgi.dll"}},
    {GraphicsAPI::VULKAN, {"vulkan-1.dll"}}
};

const __readonly QK_API std::vector amd64_external_call_signature = {"FF 15 ?? ?? ?? ??"_sig};
const __readonly QK_API std::vector arm64_external_call_signature = {
    "90 ?? ?? ??"_sig, "F9 ?? ?? ??"_sig, "D6 3F ?? ??"_sig
};
const __readonly QK_API std::vector mips_external_call_signature = {
    "3C ?? ?? ??"_sig, "03 20 ?? ??"_sig
};
const __readonly QK_API std::vector riscv_external_call_signature = {
    "?? ?? ?? 17"_sig, "?? ?? ?? 67"_sig
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
QK_API bool read_image(const std::string& image_name, Process* proc);
QK_API bool setup_process(DWORD pid, Process* proc);
QK_API bool setup_process(std::string ident, const bool& is_proc_name, Process* proc);
QK_API LPVOID alloc_page_in_proc(Process* proc, DWORD prot, size_t size = 4096);
QK_API bool inject_lib(const std::string& path, Process* proc);

// find functions
QK_API std::uintptr_t find_pattern(const byte_vec& pattern, const bool& relative, Process* proc);
QK_API std::uintptr_t find_pattern(const byte_vec& pattern, const bool& relative, Image* image);
QK_API std::uintptr_t find_pattern(
    const byte_vec& pattern, const bool& relative, const std::string& image_name, Process* proc
);

// graphics related functions
[[deprecated("this api is unfinished, and sohuld not be used")]] QK_API GraphicsInfo
find_graphics_api(Process* proc);
/// call after find_graphics_api to get the address of the active graphics context
[[deprecated("this api is unfinished, and sohuld not be used")]] QK_API GraphicsInfo
find_graphics_ctx(Process* proc, GraphicsInfo info);

}  // namespace qk::runtime::proc

#endif

#endif  // PROCESS_H
