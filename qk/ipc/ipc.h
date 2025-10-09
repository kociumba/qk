#ifndef IPC_H
#define IPC_H

#ifdef QK_IPC

#include <nng/nng.h>
#include <nng/protocol/bus0/bus.h>
#include <nng/protocol/pair0/pair.h>
#include <atomic>
#include <format>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <ranges>
#include <string>
#include <vector>
#include "../api.h"

/// implements a universal non-blocking ipc framework, that can do 1 to 1 and many to many
/// communication
namespace qk::ipc {

using log_cb = std::function<void(const std::string& qk_error_msg, const char* nng_error_string)>;
;

inline void default_error_cb(const std::string& msg, const char* nng_err) {
    if (nng_err != nullptr) {
        std::cout << "[erro] " << msg << ": " << nng_err << std::endl;
    } else {
        std::cout << "[erro] " << msg << std::endl;
    }
}

inline void default_warn_cb(const std::string& msg, const char* nng_err) {
    if (nng_err != nullptr) {
        std::cout << "[warn] " << msg << ": " << nng_err << std::endl;
    } else {
        std::cout << "[warn] " << msg << std::endl;
    }
}

/// this is the main structure providing ipc functionality, this is used the same on the server and
/// client sides
///
/// all the internals are exposed so building custom consumers for this type is encouraged if custom
/// features are needed
struct QK_API IPC {
    /// determines whether the 'IPC' will use the PAIR or BUS protocols from nng, this in turn
    /// affects the capabilities of the ipc setup
    ///
    ///     - PAIR: will only accept one pair of connections, meaning only 2 instances of 'IPC' can
    ///     be connected using this protocol
    ///     - BUS: will create a peer to peer mesh connection, with an arbitrary many 'IPC'
    ///     instances, where each one receives and sends to and from each other
    enum class Proto { PAIR, BUS } proto;
    struct IPC_Options {
        int timeout = 30000;
        int reconnect = 100;
    } opts;
    std::atomic_bool running = false;
    std::atomic_bool sending = false;
    std::string endpoint;
    std::vector<std::string> peers;
    std::vector<nng_dialer> dialers;
    std::vector<nng_listener> listeners;
    nng_socket sock;
    nng_aio *in_aio, *out_aio;
    std::mutex in_mutex, out_mutex;
    std::queue<std::string> in_bound, out_bound;

    log_cb error_cb = default_error_cb;
    log_cb warn_cb = default_warn_cb;
};

/// defines which side of an ipc connection and 'IPC' should identify as
///
///     - ANY: 'IPC' will try to connect to the endpoint, if the connection fails it will open it as
///     the server and wait for connections
///     - CLIENT: 'IPC' will only try to connect to the endpoint
///     - SERVER: 'IPC' will only try to open the endpoint as a server waiting for connections
enum class QK_API Side { ANY, CLIENT, SERVER };

QK_API void set_opts(const IPC::IPC_Options& options, IPC* ipc);

/// starts an 'IPC' instance, based on the protocol and the side this function will have drastically
/// different behaviour, read the docs on [Side] and [Proto] to see what effects they have
QK_API bool start(
    const std::string& endpoint, IPC* ipc, IPC::Proto protocol = IPC::Proto::PAIR,
    Side side = Side::ANY
);

/// stops the 'IPC' is there is any open connection associated with it, does not destroy the 'IPC'
/// object
QK_API bool stop(IPC* ipc);

/// when using 'BUS' protocol, this function adds another peer to the created mesh network, and
/// errors out if using the 'PAIR' protocol
QK_API bool add_mesh_peer(const std::string& endpoint, IPC* ipc);

/// when using the 'BUS' protocol, this functions removes a peer from the mesh network it is
/// connected to, and errors out if using 'PAIR'
QK_API bool remove_mesh_peer(const std::string& endpoint, IPC* ipc);

/// sends a message on the open connection associated with the 'IPC' object, sends to all peers if
/// using the 'BUS' protocol
QK_API bool send(const std::string& msg, IPC* ipc);

/// dequeues a received message for processing, if there are no message left return 'false'
QK_API bool dequeue_received(std::string* out, IPC* ipc);

// pass a NULL callback to reset it to the default callback
//
// the secondary argument of the callback may be NULL, and should not be expected to always be a
// string
QK_API inline void set_error_cb(const log_cb& cb, IPC* ipc) {
    if (cb == nullptr)
        ipc->error_cb = default_error_cb;
    else
        ipc->error_cb = cb;
}

// pass a NULL callback to reset it to the default callback
//
// the secondary argument of the callback may be NULL, and should not be expected to always be a
// string
QK_API inline void set_warn_cb(const log_cb& cb, IPC* ipc) {
    if (cb == nullptr)
        ipc->warn_cb = default_warn_cb;
    else
        ipc->warn_cb = cb;
}

}  // namespace qk::ipc

#endif

#endif  // IPC_H
