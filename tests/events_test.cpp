#include <qk/qk_events.h>
#include <catch2/catch_test_macros.hpp>

using namespace qk::events;

TEST_CASE("EventBus subscription and publishing", "[events]") {
    EventBus bus;

    SECTION("Subscribe and publish event") {
        bool event_triggered = false;
        auto callback = [&event_triggered](void* event) { event_triggered = true; };

        int id = subscribe<int>(callback, &bus);
        REQUIRE(id > 0);

        publish(42, &bus);
        REQUIRE(event_triggered);
    }

    SECTION("Unsubscribe specific subscriber") {
        bool event_triggered = false;
        auto callback = [&event_triggered](void* event) { event_triggered = true; };

        int id = subscribe<int>(callback, &bus);
        unsubscribe(id, &bus);

        publish(42, &bus);
        REQUIRE_FALSE(event_triggered);
    }

    SECTION("Unsubscribe all subscribers") {
        bool event_triggered = false;
        auto callback = [&event_triggered](void* event) { event_triggered = true; };

        subscribe<int>(callback, &bus);
        unsubscribe_all(&bus);

        publish(42, &bus);
        REQUIRE_FALSE(event_triggered);
    }

    SECTION("Remove event type") {
        bool event_triggered = false;
        auto callback = [&event_triggered](void* event) { event_triggered = true; };

        subscribe<int>(callback, &bus);
        remove_event<int>(&bus);

        publish(42, &bus);
        REQUIRE_FALSE(event_triggered);
    }

    SECTION("Multiple subscribers for same event") {
        std::atomic_int trigger_count = 0;
        auto callback = [&trigger_count](void* event) { ++trigger_count; };
        auto callback2 = [&trigger_count](void* event) { ++trigger_count; };

        subscribe<int>(callback, &bus);
        subscribe<int>(callback2, &bus);

        publish(42, &bus);
        REQUIRE(trigger_count == 2);
    }
}