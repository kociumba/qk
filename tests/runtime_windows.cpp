#include <qk/qk_runtime.h>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string_view>
#include <vector>

using namespace qk::runtime::mem;
using namespace qk::runtime::proc;

constexpr unsigned char test_pattern_bytes[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE};
constexpr size_t test_pattern_size = sizeof(test_pattern_bytes);

TEST_CASE("Process Utilities Windows API", "[process][memory]") {
    SECTION("Parse signature with valid hex string") {
        byte_vec result;
        std::string_view sig = "DE AD BE EF ?? ?? 12 34";

        REQUIRE(parse_signature(sig, result));
        REQUIRE(result.size() == 8);
        REQUIRE(result[0] == std::byte{0xDE});
        REQUIRE(result[1] == std::byte{0xAD});
        REQUIRE(result[2] == std::byte{0xBE});
        REQUIRE(result[3] == std::byte{0xEF});
        REQUIRE(result[4] == std::byte{WILDCARD_BYTE});
        REQUIRE(result[5] == std::byte{WILDCARD_BYTE});
        REQUIRE(result[6] == std::byte{0x12});
        REQUIRE(result[7] == std::byte{0x34});
    }

    SECTION("Parse signature with invalid hex string") {
        byte_vec result;
        std::string_view sig = "DE AD ZZ EF";

        REQUIRE_FALSE(parse_signature(sig, result));
        REQUIRE(result.size() == 2);  // DE and AD are valid hex so should be parsed
    }

    SECTION("Parse signature with only wildcards") {
        byte_vec result;
        std::string_view sig = "?? ?? ??";

        REQUIRE(parse_signature(sig, result));
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == std::byte{WILDCARD_BYTE});
        REQUIRE(result[1] == std::byte{WILDCARD_BYTE});
        REQUIRE(result[2] == std::byte{WILDCARD_BYTE});
    }

    SECTION("Parse signature with empty string") {
        byte_vec result;
        std::string_view sig = "";

        REQUIRE(parse_signature(sig, result));
        REQUIRE(result.empty());
    }

    SECTION("User-defined literal for signature") {
        auto result = "DE AD BE EF ?? 12 34"_sig;
        REQUIRE(result.size() == 7);
        REQUIRE(result[0] == std::byte{0xDE});
        REQUIRE(result[1] == std::byte{0xAD});
        REQUIRE(result[2] == std::byte{0xBE});
        REQUIRE(result[3] == std::byte{0xEF});
        REQUIRE(result[4] == std::byte{WILDCARD_BYTE});
        REQUIRE(result[5] == std::byte{0x12});
        REQUIRE(result[6] == std::byte{0x34});
    }

    SECTION("Find pattern in image with exact match") {
        Image image;
        image.base = 0x1000;
        image.bytes = {std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}, std::byte{0xDD},
                       std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};
        byte_vec pattern = {std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};

        auto addr = find_pattern(pattern, false, &image);
        REQUIRE(addr == 0x1000 + 4);
    }

    SECTION("Find pattern in image with wildcards") {
        Image image;
        image.base = 0x1000;
        image.bytes = {std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}, std::byte{0xDD},
                       std::byte{0xDE}, std::byte{0xFF}, std::byte{0xBE}, std::byte{0xEF}};
        byte_vec pattern = {
            std::byte{0xDE}, std::byte{WILDCARD_BYTE}, std::byte{0xBE}, std::byte{0xEF}
        };

        auto addr = find_pattern(pattern, true, &image);
        REQUIRE(addr == 4);
    }

    SECTION("Find pattern with no match") {
        Image image;
        image.base = 0x1000;
        image.bytes = {std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}};
        byte_vec pattern = {std::byte{0xDE}, std::byte{0xAD}};

        auto addr = find_pattern(pattern, false, &image);
        REQUIRE(addr == 0);
    }

    SECTION("Find pattern with empty pattern") {
        Image image;
        image.base = 0x1000;
        image.bytes = {std::byte{0xAA}, std::byte{0xBB}};
        byte_vec pattern;

        auto addr = find_pattern(pattern, false, &image);
        REQUIRE(addr == 0);
    }

    // self testing is iffy, idk if it actually works
    // SECTION("Setup process using current PID (self)") {
    //     DWORD current_pid = GetCurrentProcessId();
    //     Process proc;
    //
    //     REQUIRE(setup_process(current_pid, &proc));
    //     REQUIRE(proc.process != INVALID_HANDLE_VALUE);
    //     REQUIRE(proc.pid == current_pid);
    //     REQUIRE(!proc.images.empty());
    // }
    //
    // SECTION("Refresh image map for self") {
    //     DWORD current_pid = GetCurrentProcessId();
    //     Process proc;
    //     REQUIRE(setup_process(current_pid, &proc));
    //
    //     REQUIRE(refresh_image_map(&proc));
    //     REQUIRE(!proc.images.empty());
    //
    //     std::string main_module = "tests.exe";
    //     REQUIRE(proc.images.contains(main_module));
    //     REQUIRE(proc.images.contains("ntdll.dll"));
    // }
    //
    // SECTION("Read image for self (main module)") {
    //     DWORD current_pid = GetCurrentProcessId();
    //     Process proc;
    //     REQUIRE(setup_process(current_pid, &proc));
    //
    //     std::string main_module = "tests.exe";
    //     byte_vec bytes;
    //     REQUIRE(read_image(&bytes, main_module, &proc));
    //     REQUIRE(bytes.size() == proc.images[main_module]->size);
    //     REQUIRE(!bytes.empty());
    //
    //     REQUIRE(static_cast<unsigned char>(bytes[0]) == 0x4D);
    //     REQUIRE(static_cast<unsigned char>(bytes[1]) == 0x5A);
    // }
    //
    // SECTION("Find pattern in self (known pattern in main module)") {
    //     DWORD current_pid = GetCurrentProcessId();
    //     Process proc;
    //     REQUIRE(setup_process(current_pid, &proc));
    //
    //     byte_vec pattern;
    //     pattern.reserve(test_pattern_size);
    //     for (unsigned char test_pattern_byte : test_pattern_bytes) {
    //         pattern.push_back(std::byte{test_pattern_byte});
    //     }
    //
    //     std::string main_module = "tests.exe";
    //
    //     std::uintptr_t addr = find_pattern(pattern, false, main_module, &proc);
    //     REQUIRE(addr != 0);
    //
    //     const auto* direct_ptr = reinterpret_cast<const unsigned char*>(addr);
    //     bool matches = true;
    //     for (size_t i = 0; i < test_pattern_size; ++i) {
    //         if (direct_ptr[i] != test_pattern_bytes[i]) {
    //             matches = false;
    //             break;
    //         }
    //     }
    //     REQUIRE(matches);
    //
    //     std::uintptr_t rel_addr = find_pattern(pattern, true, main_module, &proc);
    //     REQUIRE(rel_addr != 0);
    //     REQUIRE(rel_addr == addr - proc.images[main_module]->base);
    // }
    //
    // SECTION("Find pattern with wildcards in self") {
    //     DWORD current_pid = GetCurrentProcessId();
    //     Process proc;
    //     REQUIRE(setup_process(current_pid, &proc));
    //
    //     byte_vec pattern = {std::byte{0xDE}, std::byte{0xAD}, std::byte{WILDCARD_BYTE},
    //                         std::byte{0xEF}, std::byte{0xCA}, std::byte{0xFE}};
    //
    //     std::string main_module = "tests.exe";
    //     std::uintptr_t addr = find_pattern(pattern, false, main_module, &proc);
    //     REQUIRE(addr != 0);
    // }
    //
    // SECTION("Alloc page in self") {
    //     DWORD current_pid = GetCurrentProcessId();
    //     Process proc;
    //     REQUIRE(setup_process(current_pid, &proc));
    //
    //     size_t alloc_size = 4096;
    //     LPVOID alloc_ptr = alloc_page_in_proc(&proc, PAGE_READWRITE, alloc_size);
    //     REQUIRE(alloc_ptr != nullptr);
    //
    //     auto test_data = "HelloTest";
    //     memcpy(alloc_ptr, test_data, strlen(test_data) + 1);
    //     REQUIRE(strcmp(static_cast<const char*>(alloc_ptr), test_data) == 0);
    //
    //     VirtualFreeEx(proc.process, alloc_ptr, 0, MEM_RELEASE);
    // }
}