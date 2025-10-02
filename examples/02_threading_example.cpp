/// this example shows how to use the qk::threading module
///
/// the module is very simple providing a goroutine like functionality and a mostly accurate
/// implementation of go channels
///
/// the design tries to be as simple as possible, but it's not possible to keep an implementation of
/// channel as simple as I would like simple while still keeping semantics like << operators

#include <qk/qk_threading.h>
#include <print>

using namespace qk::threading;

int main() {
    // creating a channel is as simple as in go, you can also create buffered
    // channels: channel<T> ch(buffer_size);
    channel<int> ch;

    // these goroutines use real threads instead of green threads, but they automatically clean up
    // due to the use of std::jthread
    go([&] {
        ch << 69;

        sleep_ms(100);

        ch << 420;
    });

    // if a channel is unbuffered receiving from it blocks until there is a value to receive
    //
    // ~ can be used to receive from a channel where c++
    // needs to deduce the type of the returned value
    //
    // this receives the value as an optional<T>
    std::print("received data: {}\n", (~ch).value());

    // otherwise << can be used to push into the channel: ch << value
    // or to receive from it when the type does not need to be deduced: variable << ch
    int recv;
    recv << ch;

    std::print("received data: {}\n", recv);

    channel<int> ch2(10);

    // if a buffered channels it overfilled, the subsequent sends will block until the channel space
    // is freed up, or it is closed
    ch2 << 69;
    ch2 << 420;
    ch2 << 2137;

    // channels have iterator support, meaning buffered channels can be iterated over to get all the
    // queued values
    for (auto val : ch2) {
        std::print("received data: {}\n", val);
    }

    return 0;
}