#include "process.h"

#ifdef QK_RUNTIME_UTILS

namespace qk::runtime::proc {

bool refresh_image_map(Process* proc) {
    MODULEENTRY32 me32 = {sizeof(MODULEENTRY32)};

    auto snapshot_handle =
        CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, proc->pid);
    if (!snapshot_handle || snapshot_handle == INVALID_HANDLE_VALUE) return false;

    proc->images.clear();

    if (Module32First(snapshot_handle, &me32)) {
        do {
            auto it = proc->images.find(me32.szModule);
            if (it == proc->images.end()) continue;
            if (it->second == nullptr) continue;

            auto image_base = reinterpret_cast<std::uintptr_t>(me32.modBaseAddr);
            auto image_size = static_cast<size_t>(me32.modBaseSize);
            proc->images[me32.szModule] =
                std::make_unique<Image>(Image{image_base, image_size, {}, proc->is64});

            if (!read_image(&proc->images[me32.szModule]->bytes, me32.szModule, proc))
                proc->images.erase(me32.szModule);
        } while (Module32Next(snapshot_handle, &me32));
    }

    CloseHandle(snapshot_handle);

    return true;
}

bool read_image(byte_vec* dest, const std::string& image_name, Process* proc) {
    if (!dest || image_name.empty() || !proc->images.contains(image_name)) return false;

    auto image = proc->images.at(image_name).get();
    dest->clear();
    dest->resize(image->size);

    return ReadProcessMemory(
               proc->process, reinterpret_cast<LPCVOID>(image->base), dest->data(), image->size,
               nullptr
           ) != 0;
}

bool read_image(const std::string& image_name, Process* proc) {
    if (!image_name.empty() || !proc->images.contains(image_name)) return false;
    return read_image(&proc->images[image_name]->bytes, image_name, proc);
}

struct window_cb_args {
    DWORD target_pid;
    HWND target_hwnd;
};

BOOL CALLBACK hwnd_cb(HWND hWnd, LPARAM lparam) {
    DWORD pid = DWORD();

    GetWindowThreadProcessId(hWnd, &pid);

    const auto args = reinterpret_cast<window_cb_args*>(lparam);

    if (pid == args->target_pid) {
        args->target_hwnd = hWnd;

        return FALSE;
    }

    return TRUE;
};

bool setup_process(const DWORD pid, Process* proc) {
    auto handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (!handle) return false;

    BOOL wow64;
    proc->is64 = IsWow64Process(handle, &wow64) && !wow64;

    window_cb_args args = {pid, HWND()};

    EnumWindows(hwnd_cb, reinterpret_cast<LPARAM>(&args));

    // if (!args.target_hwnd) return false;

    proc->hwnd = args.target_hwnd;
    proc->pid = pid;
    proc->process = handle;

    if (!refresh_image_map(proc)) {
        proc->hwnd = nullptr;
        proc->pid = 0;
        proc->process = INVALID_HANDLE_VALUE;

        return false;
    }

    return true;
}

bool setup_process(std::string ident, const bool& is_proc_name, Process* proc) {
    if (ident.empty()) return false;
    auto w_ident = to_wstring(ident);

    auto window_handle = HWND();
    auto buffer = DWORD();
    auto proc_handle = INVALID_HANDLE_VALUE;

    if (!is_proc_name) {
        window_handle = FindWindowW(nullptr, w_ident.c_str());
        if (!window_handle) return false;

        if (!GetWindowThreadProcessId(window_handle, &buffer)) return false;

        proc_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, buffer);
        if (!proc_handle) return false;
    } else {
        const auto snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (!snapshot_handle) return false;

        auto pe32 = PROCESSENTRY32();
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(snapshot_handle, &pe32)) {
            do {
                if (const auto wprocess_name = to_wstring(pe32.szExeFile);
                    wprocess_name == w_ident) {
                    buffer = pe32.th32ProcessID;

                    break;
                }
            } while (Process32Next(snapshot_handle, &pe32));
        } else
            return false;

        proc_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, buffer);
        if (!proc_handle) return false;

        CloseHandle(snapshot_handle);

        BOOL wow64;
        proc->is64 = IsWow64Process(proc_handle, &wow64) && !wow64;

        window_cb_args args = {buffer, HWND()};

        EnumWindows(hwnd_cb, reinterpret_cast<LPARAM>(&args));

        // if (!args.target_hwnd) return false;

        window_handle = args.target_hwnd;
    }

    proc->hwnd = window_handle;
    proc->pid = buffer;
    proc->process = proc_handle;

    if (!refresh_image_map(proc)) {
        proc->hwnd = nullptr;
        proc->pid = 0;
        proc->process = INVALID_HANDLE_VALUE;

        return false;
    }

    return true;
}

LPVOID alloc_page_in_proc(Process* proc, DWORD prot, size_t size) {
    const auto ret = VirtualAllocEx(proc->process, nullptr, size, MEM_COMMIT | MEM_RESERVE, prot);

    return ret;
}

bool inject_lib(const std::string& path, Process* proc) {
    const auto mem_lib = alloc_page_in_proc(proc, PAGE_READWRITE, path.size() + 1);
    if (!mem_lib) return false;

    if (!WriteProcessMemory(proc->process, mem_lib, path.c_str(), path.size() + 1, nullptr))
        return false;

    const auto module_handle = GetModuleHandleA("kernel32.dll");
    if (!module_handle) return false;

    const auto loadlib_a = (LPVOID)GetProcAddress(module_handle, "LoadLibraryA");
    if (!loadlib_a) return false;

    const auto remote_thread = CreateRemoteThread(
        proc->process, nullptr, NULL, (LPTHREAD_START_ROUTINE)loadlib_a, mem_lib, NULL, nullptr
    );
    if (!remote_thread) return false;

    WaitForSingleObject(remote_thread, INFINITE);
    CloseHandle(remote_thread);

    VirtualFreeEx(proc->process, mem_lib, NULL, MEM_RELEASE);

    return true;
}

////////////////////////////////////
//                                //
//        IMAGE FUNCTIONS         //
//                                //
////////////////////////////////////

std::uintptr_t find_pattern(const byte_vec& pattern, const bool& relative, Process* proc) {
    bool found = false;
    std::uintptr_t addr = 0;
    Image* containing_image = nullptr;

    for (auto& image : proc->images | std::views::values) {
        if (found) break;

        if (addr = find_pattern(pattern, relative, image.get()); addr != 0) {
            containing_image = image.get();
            found = true;
        }
    }

    if (!addr || !containing_image) return 0;

    return addr;
}

std::uintptr_t find_pattern(const byte_vec& pattern, const bool& relative, Image* image) {
    bool found = false;
    std::uintptr_t addr = 0;

    for (std::uintptr_t current_adr = 0; current_adr < image->bytes.size(); current_adr++) {
        if (found) break;

        for (uint8_t i = 0; i < pattern.size(); i++) {
            auto current_byte = image->bytes[current_adr + i];
            auto pattern_byte = pattern[i];

            if (static_cast<uint8_t>(pattern_byte) == WILDCARD_BYTE) {
                if (i == pattern.size() - 1) {
                    found = true;
                    addr = current_adr;
                    break;
                }
                continue;
            }

            if (current_byte != pattern_byte) {
                break;
            }

            if (i == pattern.size() - 1) {
                found = true;
                addr = current_adr;
            }
        }
    }

    if (!addr) return 0;

    return relative ? addr : image->base + addr;
}

std::uintptr_t find_pattern(
    const byte_vec& pattern, const bool& relative, const std::string& image_name, Process* proc
) {
    if (!proc->images.contains(image_name)) return 0;

    return find_pattern(pattern, relative, proc->images[image_name].get());
}

GraphicsInfo find_graphics_api(Process* proc) {
    if (!refresh_image_map(proc)) return {};

    for (const auto& [api, modules] : api_modules) {
        for (const auto& module : modules) {
            if (proc->images.contains(module)) return {api, 0, module};
        }
    }

    return {};
}

GraphicsInfo find_graphics_ctx(Process* proc, GraphicsInfo info) {
    if (!info.valid()) return {};

    if (!read_image(info.module_name, proc)) return {};

    return {};
}

}  // namespace qk::runtime::proc

#endif