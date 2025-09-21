#include "events.h"

namespace qk::events {

// int subscribe(event_cb callback, reflect::detail::any event_type, EventBus* bus) {
//     std::lock_guard l(bus->mu);
//
//     Subscriber sub{};
//     sub.cb = callback;
//
//     if (!bus->subscribers.contains(reflect::type_id(event_type))) {
//         sub.id = ++bus->id_counter;
//
//         bus->subscribers[reflect::type_id(event_type)].emplace_back(sub);
//     }
//
//     return sub.id;
// }

QK_API void unsubscribe(int id, EventBus* bus) {
    std::lock_guard l(bus->mu);

    for (const auto& [event, subs] : bus->subscribers) {
        for (size_t i = 0; i < subs.size(); i++) {
            if (subs[i].id == id) {
                bus->subscribers[event].erase(subs.begin() + i);
                return;
            }
        }
    }
}

QK_API void unsubscribe_all(EventBus* bus) {
    std::lock_guard l(bus->mu);

    bus->subscribers.clear();
}

// void remove_event(reflect::detail::any event_type, EventBus* bus) {
//     std::lock_guard l(bus->mu);
//
//     bus->subscribers.erase(reflect::type_id(event_type));
// }

}  // namespace qk::events