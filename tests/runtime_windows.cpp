#include <qk/qk_runtime.h>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string_view>
#include <vector>

using namespace qk::runtime::mem;
using namespace qk::runtime::proc;

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
}