/// this example shows how to use the qk::events module
///
/// the module is very simple it provides a single event bus type and a few functions that all
/// operate on the idea that types are events
///
/// creating an event is as simple as defining a type, literally any type, and publishing only
/// requires an instance of that type, all subscribers receive the pointer to that instance, and it
/// is their job to cast it from void* to Event*

#include <qk/qk_events.h>
#include <print>

using namespace qk::events;

struct ExampleEvent {
    int data;
};

int main() {
    EventBus eb;

    // subscribing is based on types, the core philosophy being that types ARE events
    int sub_id = subscribe<ExampleEvent>(
        [](void* e) {
            auto event = (ExampleEvent*)e;
            std::print("got event: {}\n", event->data);
        },
        &eb
    );

    // publishing a type notifies all the subscribers for that type
    publish(ExampleEvent{69}, &eb);
    publish(ExampleEvent{420}, &eb);

    // subscriber management is based on simple integer ids
    unsubscribe(sub_id, &eb);

    publish(ExampleEvent{2137}, &eb);

    return 0;
}