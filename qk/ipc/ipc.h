#ifndef IPC_H
#define IPC_H

#ifdef QK_IPC

#include <assert.h>
#include <nng/nng.h>
#include <nng/protocol/bus0/bus.h>
#include <nng/protocol/pair0/pair.h>
#include <atomic>
#include <format>
#include <iostream>
#include <mutex>
#include <queue>
#include <ranges>
#include <string>
#include <vector>
#include "../api.h"

namespace qk::ipc {

using log_cb = void (*)(const std::string& kq_error_msg, const char* nng_error_string);

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

struct QK_API IPC {
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

enum class QK_API Side { ANY, CLIENT, SERVER };

QK_API void set_opts(const IPC::IPC_Options& options, IPC* ipc);
QK_API bool start(
    const std::string& endpoint, IPC* ipc, IPC::Proto protocol = IPC::Proto::PAIR,
    Side side = Side::ANY
);
QK_API bool stop(IPC* ipc);
QK_API bool add_mesh_peer(const std::string& endpoint, IPC* ipc);
QK_API bool remove_mesh_peer(const std::string& endpoint, IPC* ipc);

QK_API bool send(const std::string& msg, IPC* ipc);
QK_API bool dequeue_received(std::string* out, IPC* ipc);

// pass a NULL callback to reset it to the default callback
//
// the secondary argument of the callback may be NULL, and should not be expected to always be a
// string
QK_API inline void set_error_cb(const log_cb cb, IPC* ipc) {
    if (cb == nullptr)
        ipc->error_cb = default_error_cb;
    else
        ipc->error_cb = cb;
}

// pass a NULL callback to reset it to the default callback
//
// the secondary argument of the callback may be NULL, and should not be expected to always be a
// string
QK_API inline void set_warn_cb(const log_cb cb, IPC* ipc) {
    if (cb == nullptr)
        ipc->warn_cb = default_warn_cb;
    else
        ipc->warn_cb = cb;
}

}  // namespace qk::ipc

#endif

#endif  // IPC_H
