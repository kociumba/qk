#ifndef EVENTS_H
#define EVENTS_H

#ifdef QK_EVENTS

#include <atomic>
// #include <mp>
#include <functional>
#include <mutex>
#include <ranges>
#include <reflect>
#include <unordered_map>
#include <vector>
#include "../api.h"

// the reflect::detail::any functions will probably not work, since for real type erasure I would
// need to use mp
//
// look in to that later
namespace qk::events {

using event_cb = std::function<void(void*)>;

struct QK_API Subscriber {
    event_cb cb;
    int id;
};

/// the main event bus type, used for all the event operations
struct QK_API EventBus {
    std::mutex mu;
    std::atomic_int id_counter = 0;
    std::unordered_map<size_t, std::vector<Subscriber>> subscribers;
};

/// subscribes a new subscriber to an event type, subscribers are not deduplicated
template <typename Event>
int subscribe(event_cb callback, EventBus* bus) {
    std::lock_guard l(bus->mu);

    Subscriber sub{};
    sub.cb = callback;

    sub.id = ++bus->id_counter;
    bus->subscribers[reflect::type_id<Event>()].emplace_back(sub);

    return sub.id;
}

// int subscribe(event_cb callback, reflect::detail::any event_type, EventBus* bus);

/// unsubscribes a specific subscriber using its id, the id can be retrieved when subscribing
QK_API void unsubscribe(int id, EventBus* bus);

/// unsubscribes all subscribers, essentially clearing the event bus
QK_API void unsubscribe_all(EventBus* bus);

/// removes an event type from the event bus and unsubscribes all subscribers for that type
template <typename Event>
void remove_event(EventBus* bus) {
    std::lock_guard l(bus->mu);
    bus->subscribers.erase(reflect::type_id<Event>());
}

// void remove_event(reflect::detail::any event_type, EventBus* bus);

/// publishes an event, does not check if the type has at least a single subscriber
template <typename Event>
void publish(Event event, EventBus* bus) {
    std::lock_guard l(bus->mu);

    for (const auto& sub : bus->subscribers[reflect::type_id(event)]) {
        sub.cb(&event);
    }
}

}  // namespace qk::events

#endif

#endif  // EVENTS_H
