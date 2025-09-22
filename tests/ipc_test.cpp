#include <qk/qk_ipc.h>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

using namespace qk::ipc;

TEST_CASE("IPC pair communication", "[ipc]") {
    IPC server, client;
    std::string endpoint = "inproc://test_pair";

    SECTION("Server-client communication") {
        bool server_started = start(endpoint, &server, IPC::Proto::PAIR, Side::SERVER);
        REQUIRE(server_started);

        bool client_started = start(endpoint, &client, IPC::Proto::PAIR, Side::CLIENT);
        REQUIRE(client_started);

        std::string sent_msg = "Hello, IPC!";
        REQUIRE(send(sent_msg, &client));

        std::string received_msg;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(dequeue_received(&received_msg, &server));
        REQUIRE(received_msg == sent_msg);

        REQUIRE(stop(&client));
        REQUIRE(stop(&server));
    }
}

TEST_CASE("IPC bus communication", "[ipc]") {
    IPC node1, node2;
    std::string endpoint1 = "inproc://test_bus1";
    std::string endpoint2 = "inproc://test_bus2";

    SECTION("Mesh peer communication") {
        REQUIRE(start(endpoint1, &node1, IPC::Proto::BUS, Side::SERVER));
        REQUIRE(start(endpoint2, &node2, IPC::Proto::BUS, Side::SERVER));

        REQUIRE(add_mesh_peer(endpoint2, &node1));
        REQUIRE(add_mesh_peer(endpoint1, &node2));

        std::string sent_msg = "Bus message";
        REQUIRE(send(sent_msg, &node1));

        std::string received_msg;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(dequeue_received(&received_msg, &node2));
        REQUIRE(received_msg == sent_msg);

        REQUIRE(remove_mesh_peer(endpoint1, &node2));
        REQUIRE(stop(&node1));
        REQUIRE(stop(&node2));
    }
}

TEST_CASE("IPC error handling", "[ipc]") {
    IPC ipc;
    std::string invalid_endpoint = "inproc://invalid_endpoint";

    SECTION("Invalid endpoint") {
        bool error_triggered = false;
        set_error_cb(
            [&error_triggered](const std::string&, const char*) { error_triggered = true; }, &ipc
        );

        REQUIRE_FALSE(start(invalid_endpoint, &ipc, IPC::Proto::PAIR, Side::CLIENT));
        REQUIRE(error_triggered);
    }

    SECTION("Custom warning callback") {
        bool warning_triggered = false;
        set_warn_cb(
            [&warning_triggered](const std::string&, const char*) { warning_triggered = true; },
            &ipc
        );

        REQUIRE(start(invalid_endpoint, &ipc, IPC::Proto::BUS, Side::ANY));
        REQUIRE_FALSE(add_mesh_peer(invalid_endpoint, &ipc));
        REQUIRE(warning_triggered);
        REQUIRE(stop(&ipc));
    }
}