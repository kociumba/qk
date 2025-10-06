#include "ipc.h"

#ifdef QK_IPC

namespace qk::ipc {

void in_func(void* arg) {
    auto ipc = (IPC*)arg;
    int err = 0;

    if ((err = nng_aio_result(ipc->in_aio)) == 0) {
        auto msg = nng_aio_get_msg(ipc->in_aio);

        std::lock_guard l(ipc->in_mutex);
        ipc->in_bound.emplace((char*)nng_msg_body(msg), nng_msg_len(msg));

        nng_msg_free(msg);
    } else {
        ipc->warn_cb("receiving inbound message failed", nng_strerror(err));
    }

    if (ipc->running) {
        nng_recv_aio(ipc->sock, ipc->in_aio);
    }
}

void out_func(void* arg) {
    auto ipc = (IPC*)arg;
    int err = nng_aio_result(ipc->out_aio);
    if (err != 0) {
        ipc->warn_cb("sending outbound message failed", nng_strerror(err));
        if (auto msg = nng_aio_get_msg(ipc->out_aio)) nng_msg_free(msg);
    }

    nng_msg* next_msg = nullptr;
    bool has_more = false;

    {
        std::lock_guard l(ipc->out_mutex);
        if (!ipc->out_bound.empty()) {
            std::string next = std::move(ipc->out_bound.front());
            ipc->out_bound.pop();

            err = nng_msg_alloc(&next_msg, 0);
            if (err == 0) {
                err = nng_msg_append(next_msg, next.data(), next.size());
            }

            if (err != 0) {
                ipc->warn_cb(
                    "message allocation filed while sending outbound message", nng_strerror(err)
                );
                if (next_msg) nng_msg_free(next_msg);
                next_msg = nullptr;
            } else {
                has_more = true;
            }
        }

        if (!has_more) {
            ipc->sending = false;
        }
    }

    if (has_more) {
        nng_aio_set_msg(ipc->out_aio, next_msg);
        nng_send_aio(ipc->sock, ipc->out_aio);
    }
}

QK_API void set_opts(const IPC::IPC_Options& options, IPC* ipc) { ipc->opts = options; }

QK_API bool start(const std::string& endpoint, IPC* ipc, IPC::Proto protocol, Side side) {
    int err = 0;
    ipc->proto = protocol;
    switch (ipc->proto) {
        case IPC::Proto::PAIR:
            if ((err = nng_pair0_open(&ipc->sock)) != 0) {
                ipc->error_cb("failed to create a pair socket", nng_strerror(err));
                return false;
            }
            break;
        case IPC::Proto::BUS:
            if ((err = nng_bus0_open(&ipc->sock)) != 0) {
                ipc->error_cb("failed to create a bus socket", nng_strerror(err));
                return false;
            }
            break;
    }

    bool connection_established = false;
    if (side == Side::SERVER) {
        nng_listener listener;
        if ((err = nng_listener_create(&listener, ipc->sock, endpoint.c_str())) != 0) {
            ipc->error_cb("failed to create listener on " + endpoint, nng_strerror(err));
            nng_close(ipc->sock);
            return false;
        }

        if ((err = nng_listener_start(listener, 0)) != 0) {
            ipc->error_cb("failed to start listener on " + endpoint, nng_strerror(err));
            nng_listener_close(listener);
            nng_close(ipc->sock);
            return false;
        }

        ipc->listeners.push_back(listener);

        connection_established = true;

    } else if (side == Side::CLIENT) {
        nng_dialer dialer;
        if ((err = nng_dialer_create(&dialer, ipc->sock, endpoint.c_str())) != 0) {
            ipc->error_cb("failed to create dialer for " + endpoint, nng_strerror(err));
            nng_close(ipc->sock);
            return false;
        }

        nng_dialer_set_ms(dialer, NNG_OPT_RECONNMINT, ipc->opts.reconnect);
        nng_dialer_set_ms(dialer, NNG_OPT_RECONNMAXT, ipc->opts.timeout);

        if ((err = nng_dialer_start(dialer, 0)) != 0) {
            ipc->error_cb("failed to dial the requested endpoint", nng_strerror(err));
            nng_dialer_close(dialer);
            nng_close(ipc->sock);
            return false;
        }

        ipc->dialers.push_back(dialer);
        ipc->peers.push_back(endpoint);

        connection_established = true;

    } else {
        nng_dialer dialer;
        if ((err = nng_dialer_create(&dialer, ipc->sock, endpoint.c_str())) != 0) {
            ipc->error_cb("failed to create dialer for " + endpoint, nng_strerror(err));
            nng_close(ipc->sock);
            return false;
        }

        nng_dialer_set_ms(dialer, NNG_OPT_RECONNMINT, ipc->opts.reconnect);
        nng_dialer_set_ms(dialer, NNG_OPT_RECONNMAXT, ipc->opts.timeout);

        err = nng_dialer_start(dialer /*, NNG_FLAG_NONBLOCK*/, 0);
        if (err == 0) {
            ipc->dialers.push_back(dialer);
            ipc->peers.push_back(endpoint);

            connection_established = true;

        } else if (err == NNG_ECONNREFUSED || err == NNG_ECONNRESET) {
            nng_dialer_close(dialer);

            nng_listener listener;
            if ((err = nng_listener_create(&listener, ipc->sock, endpoint.c_str())) != 0) {
                ipc->error_cb("failed to create listener on " + endpoint, nng_strerror(err));
                nng_close(ipc->sock);
                return false;
            }

            if ((err = nng_listener_start(listener, 0)) != 0) {
                ipc->error_cb(
                    "failed to listen on " + endpoint + " after dial failed", nng_strerror(err)
                );
                nng_listener_close(listener);
                nng_close(ipc->sock);
                return false;
            }

            ipc->listeners.push_back(listener);
            connection_established = true;

        } else {
            ipc->error_cb("failed to establish connection on " + endpoint, nng_strerror(err));
            nng_dialer_close(dialer);
            nng_close(ipc->sock);
            return false;
        }
    }

    if (!connection_established) {
        nng_close(ipc->sock);
        return false;
    }

    ipc->endpoint = endpoint;

    if ((err = nng_aio_alloc(&ipc->in_aio, in_func, ipc)) != 0) {
        ipc->error_cb("failed to allocate inpout aio", nng_strerror(err));
        stop(ipc);
        return false;
    }
    if ((err = nng_aio_alloc(&ipc->out_aio, out_func, ipc)) != 0) {
        ipc->error_cb("failed to allocate output aio", nng_strerror(err));
        nng_aio_free(ipc->in_aio);
        stop(ipc);
        return false;
    }

    ipc->running = true;
    ipc->sending = false;

    // kickstart the input listener
    nng_recv_aio(ipc->sock, ipc->in_aio);

    return true;
}

QK_API bool stop(IPC* ipc) {
    if (ipc->running.exchange(false)) {
        nng_aio_cancel(ipc->in_aio);
        nng_aio_cancel(ipc->out_aio);

        nng_aio_wait(ipc->in_aio);
        nng_aio_wait(ipc->out_aio);

        nng_aio_free(ipc->in_aio);
        nng_aio_free(ipc->out_aio);

        for (const auto& dialer : ipc->dialers) {
            nng_dialer_close(dialer);
        }
        ipc->dialers.clear();

        for (const auto& listener : ipc->listeners) {
            nng_listener_close(listener);
        }
        ipc->listeners.clear();

        ipc->peers.clear();

        nng_close(ipc->sock);

        ipc->sending = false;
        ipc->endpoint = "";

        return true;
    }

    return false;
}

bool add_mesh_peer(const std::string& endpoint, IPC* ipc) {
    if (ipc->proto != IPC::Proto::BUS) {
        ipc->error_cb("mesh peers are only supported with the bus protocol", nullptr);
        return false;
    }

    if (!ipc->running) {
        ipc->error_cb("cannot add peers to a stopped IPC instance", nullptr);
        return false;
    }

    auto it = std::ranges::find(ipc->peers, endpoint);
    if (ipc->endpoint == endpoint || it != ipc->peers.end()) {
        ipc->warn_cb("already connected to endpoint: " + endpoint, nullptr);
        return false;
    }

    nng_dialer dialer;
    int err = nng_dialer_create(&dialer, ipc->sock, endpoint.c_str());
    if (err != 0) {
        ipc->error_cb("failed to create dialer for " + endpoint, nng_strerror(err));
        return false;
    }

    nng_dialer_set_ms(dialer, NNG_OPT_RECONNMINT, ipc->opts.reconnect);
    nng_dialer_set_ms(dialer, NNG_OPT_RECONNMAXT, ipc->opts.timeout);

    err = nng_dialer_start(dialer, /* NNG_FLAG_NONBLOCK */ 0);
    if (err != 0 && err != NNG_ECONNREFUSED) {
        ipc->error_cb("failed to start dialer for " + endpoint, nng_strerror(err));
        nng_dialer_close(dialer);
        return false;
    }

    ipc->dialers.push_back(std::move(dialer));
    ipc->peers.push_back(std::move(endpoint));

    return true;
}

bool remove_mesh_peer(const std::string& endpoint, IPC* ipc) {
    if (ipc->proto != IPC::Proto::BUS) {
        ipc->error_cb("mesh peers are only supported with the bus protocol", nullptr);
        return false;
    }

    auto it = std::ranges::find(ipc->peers, endpoint);
    if (it == ipc->peers.end()) {
        ipc->warn_cb("not connected to endpoint: " + endpoint, nullptr);
        return false;
    }

    size_t index = std::distance(ipc->peers.begin(), it);
    if (index < ipc->dialers.size()) {
        nng_dialer_close(ipc->dialers[index]);
        ipc->dialers.erase(ipc->dialers.begin() + index);
    }

    ipc->peers.erase(it);

    return true;
}

QK_API bool send(const std::string& msg, IPC* ipc) {
    nng_msg* prep_msg = nullptr;
    bool start_send = false;
    int err = 0;

    {
        std::lock_guard l(ipc->out_mutex);
        ipc->out_bound.push(msg);

        if (!ipc->sending && ipc->out_bound.size() == 1) {
            ipc->sending = true;

            std::string next = std::move(ipc->out_bound.front());
            ipc->out_bound.pop();

            err = nng_msg_alloc(&prep_msg, 0);
            if (err == 0) {
                err = nng_msg_append(prep_msg, next.data(), next.size());
            }

            if (err != 0) {
                ipc->warn_cb(
                    "message allocation failed while sending an outbound message", nng_strerror(err)
                );
                if (prep_msg) nng_msg_free(prep_msg);
                ipc->sending = false;
                return false;
            }

            start_send = true;
        }
    }

    if (start_send) {
        nng_aio_set_msg(ipc->out_aio, prep_msg);
        nng_send_aio(ipc->sock, ipc->out_aio);
    }

    return true;
}

QK_API bool dequeue_received(std::string* out, IPC* ipc) {
    std::lock_guard l(ipc->in_mutex);
    if (ipc->in_bound.empty()) {
        return false;
    }

    *out = std::move(ipc->in_bound.front());
    ipc->in_bound.pop();

    return true;
}

}  // namespace qk::ipc

#endif