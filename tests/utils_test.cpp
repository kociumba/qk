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

    // SECTION("partial application") {
    //     using namespace qk::utils;
    //
    //     auto add_4 = [](int a, int b, int c, int d) { return a + b + c + d; };
    //
    //     auto add_1_and_3 = apply(add_4, _, 69, _, 420);
    //
    //     REQUIRE(add_1_and_3(2137, 80085) == 82711);
    //
    //     auto add_1 = apply(add_1_and_3, _, 80085);
    //
    //     REQUIRE(add_1(2137) == 82711);
    // }
}