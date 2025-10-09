#include <qk/qk_utils.h>
#include <catch2/catch_test_macros.hpp>
#include <print>

TEST_CASE("Utils testing") {
    SECTION("defer") {
        {
            int count = 0;
            defer(count++; std::println("deferred print, count: {}", count));

            count++;
            std::println("gabagool, count: {}", count);
            REQUIRE(count == 1);
        }
    }
}