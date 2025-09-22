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

// this uses reflection to use types as events
struct QK_API EventBus {
    std::mutex mu;
    std::atomic_int id_counter = 0;
    std::unordered_map<size_t, std::vector<Subscriber>> subscribers;
};

template <typename Event>
QK_API int subscribe(event_cb callback, EventBus* bus) {
    std::lock_guard l(bus->mu);

    Subscriber sub{};
    sub.cb = callback;

    sub.id = ++bus->id_counter;
    bus->subscribers[reflect::type_id<Event>()].emplace_back(sub);

    return sub.id;
}

// int subscribe(event_cb callback, reflect::detail::any event_type, EventBus* bus);

QK_API void unsubscribe(int id, EventBus* bus);
QK_API void unsubscribe_all(EventBus* bus);

template <typename Event>
QK_API void remove_event(EventBus* bus) {
    std::lock_guard l(bus->mu);
    bus->subscribers.erase(reflect::type_id<Event>());
}

// void remove_event(reflect::detail::any event_type, EventBus* bus);

template <typename Event>
QK_API void publish(Event event, EventBus* bus) {
    std::lock_guard l(bus->mu);

    for (const auto& sub : bus->subscribers[reflect::type_id(event)]) {
        sub.cb(&event);
    }
}

}  // namespace qk::events

#endif

#endif  // EVENTS_H
