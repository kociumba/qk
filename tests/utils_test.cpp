#include <qk/qk_utils.h>
#include <catch2/catch_test_macros.hpp>
#include <print>
#include <ranges>

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

    SECTION("generic stream") {
        using namespace qk::utils;
        stream<int> s;

        s << 6 << 9 << std::ranges::views::iota(3, 6);
        REQUIRE(s.size() == 5);
        std::println("stream: {}", s);
    }

    SECTION("run once") {
        using namespace qk::utils;

        int loop = 0;
        int count = 0;
        do {
            once([&] { count++; });
            loop++;
        } while (loop < 10);

        REQUIRE(loop > 1);
        REQUIRE(count == 1);
    }

    SECTION("result types") {
        using namespace qk::utils;

        auto error_thrower = [] -> Result<int, std::string> { return "error"; };
        auto ok_worker = [] -> Result<int, std::string> { return ok(69); };

        auto r = error_thrower();
        REQUIRE(!r);
        std::println("got error: {}", r.unwrap_err());

        r = ok_worker();
        REQUIRE(r);
        std::println("got value: {}", r.unwrap());

        std::println("raw value: {}", *(int*)r._get_value());
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