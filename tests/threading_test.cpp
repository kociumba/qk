#include <qk/qk_threading.h>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

using namespace qk::threading;

TEST_CASE("Goroutine execution", "[threading]") {
    SECTION("Simple goroutine") {
        bool executed = false;
        go([&executed] { executed = true; });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(executed);
    }

    SECTION("Goroutine with arguments") {
        int result = 0;
        go([](int a, int b, int* out) { *out = a + b; }, 5, 3, &result);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result == 8);
    }
}

TEST_CASE("Channel communication", "[threading]") {
    SECTION("Send and receive on unbuffered channel") {
        int_channel ch;
        int result = 0;

        go([&ch] { ch.send(42); });
        go([&ch, &result] {
            if (auto val = ch.receive()) result = *val;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(result == 42);
    }

    SECTION("Send and receive on buffered channel") {
        int_channel ch(2);
        ch.send(1);
        ch.send(2);

        auto val1 = ch.receive();
        auto val2 = ch.receive();

        REQUIRE(val1 == 1);
        REQUIRE(val2 == 2);
    }

    SECTION("Channel closure") {
        int_channel ch;
        ch.close();

        REQUIRE_FALSE(ch.send(42));
        REQUIRE(ch.is_closed());
        REQUIRE_FALSE(ch.receive().has_value());
    }

    SECTION("Channel iterator") {
        int_channel ch(3);
        go([&ch] {
            ch.send(1);
            ch.send(2);
            ch.send(3);
            ch.close();
        });

        std::vector<int> results;
        for (auto val : ch) {
            results.push_back(val);
        }

        REQUIRE(results == std::vector<int>({1, 2, 3}));
    }
}