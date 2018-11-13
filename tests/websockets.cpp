/* Copyright (c) 2017-2018, EPFL/Blue Brain Project
 *                          Raphael.Dumusc@epfl.ch
 *
 * This file is part of Rockets <https://github.com/BlueBrain/Rockets>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define BOOST_TEST_MODULE rockets_websockets

#include <rockets/helpers.h>
#include <rockets/server.h>
#include <rockets/ws/client.h>

#include <iostream>

#include <boost/mpl/vector.hpp>
#include <boost/test/unit_test.hpp>

#include <libwebsockets.h>

#define CLIENT_SUPPORTS_INEXISTANT_PROTOCOL_ERRORS \
    (LWS_LIBRARY_VERSION_NUMBER >= 2000000)

using namespace rockets;

namespace
{
const auto wsProtocol = "ws_test_protocol";

void connect(ws::Client& client, Server& server, bool skipCheck = false)
{
    auto future = client.connect(server.getURI(), wsProtocol);
    while (!is_ready(future))
    {
        client.process(10);
        if (server.getThreadCount() == 0)
            server.process(10);
    }
    if (skipCheck)
        future.get();
    else
        BOOST_REQUIRE_NO_THROW(future.get());

    if (server.getThreadCount() > 0)
        std::this_thread::sleep_for(std::chrono::microseconds(10));
}

struct ScopedEnvironment
{
    ScopedEnvironment(const std::string& key, const std::string& value)
        : _key(key)
    {
        setenv(key.c_str(), value.c_str(), 0);
    }
    ~ScopedEnvironment() { unsetenv(_key.c_str()); }
private:
    std::string _key;
};
} // anonymous namespace

BOOST_AUTO_TEST_CASE(server_construction)
{
    Server server1{"", wsProtocol};
    BOOST_CHECK_NE(server1.getURI(), "");
    BOOST_CHECK_NE(server1.getPort(), 0);
    BOOST_CHECK_EQUAL(server1.getThreadCount(), 0);
}

BOOST_AUTO_TEST_CASE(listening_on_unavailable_port_throws)
{
    BOOST_CHECK_THROW(Server(":80", wsProtocol), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(client_connection_to_inexistant_server)
{
    ws::Client client;
    auto future = client.connect("inexistantServer:453", wsProtocol);
    int attempts = 1000;
    while (!is_ready(future) && --attempts)
        client.process(10);

    BOOST_REQUIRE(is_ready(future));
    BOOST_CHECK_THROW(future.get(), std::runtime_error);
}

#if CLIENT_SUPPORTS_INEXISTANT_PROTOCOL_ERRORS
BOOST_AUTO_TEST_CASE(client_connection_to_inexistant_protocol)
{
    Server server{"", wsProtocol};
    ws::Client client;
    auto future = client.connect(server.getURI(), "inexistantProtocol");
    BOOST_REQUIRE(!is_ready(future));
    while (!is_ready(future))
    {
        client.process(10);
        server.process(10);
    }
    BOOST_CHECK_THROW(future.get(), std::runtime_error);
}
#endif

BOOST_AUTO_TEST_CASE(connect_to_localhost_with_proxy_and_no_proxy)
{
    ScopedEnvironment no_proxy("no_proxy", "127.0.0.1");
    ScopedEnvironment http_proxy("http_proxy", "proxy:12345");

    Server server("127.0.0.1:", wsProtocol);
    ws::Client client;
    BOOST_CHECK_NO_THROW(connect(client, server, true));
}

BOOST_AUTO_TEST_CASE(connect_to_localhost_with_proxy_and_no_proxy_list)
{
    ScopedEnvironment no_proxy("no_proxy", "localhost,*.0.0.1");
    ScopedEnvironment http_proxy("http_proxy", "proxy:12345");

    Server server("127.0.0.1:", wsProtocol);
    ws::Client client;
    BOOST_CHECK_NO_THROW(connect(client, server, true));
}

BOOST_AUTO_TEST_CASE(connect_to_localhost_with_proxy)
{
    ScopedEnvironment http_proxy("http_proxy", "proxy:12345");

    Server server("127.0.0.1:", wsProtocol);
    ws::Client client;
    BOOST_CHECK_THROW(connect(client, server, true), std::runtime_error);
}

/**
 * Fixtures to run all test cases with {0, 1, 2} server worker threads.
 */
struct Fixture
{
    ws::Client client1;
    ws::Client client2;
    std::unique_ptr<ws::Client> client3{new ws::Client};
    std::atomic<bool> receivedMessage1 = {false};
    std::atomic<bool> receivedMessage2 = {false};
    std::atomic<bool> receivedReply1 = {false};
    std::atomic<bool> receivedReply2 = {false};
    std::atomic<bool> receivedConnect = {false};
    std::atomic<bool> receivedConnectReply = {false};
    std::atomic<bool> receivedDisconnect = {false};

    void processClient1(Server& server)
    {
        while (!(receivedMessage1 && receivedReply1))
        {
            client1.process(5);
            if (server.getThreadCount() == 0)
                server.process(10);
        }
    }

    void processClient3Connect(Server& server)
    {
        while (!(receivedConnect && receivedConnectReply))
        {
            if (client3)
                client3->process(5);
            if (server.getThreadCount() == 0)
                server.process(10);
        }
    }

    void processClient3Disconnect(Server& server)
    {
        while (!receivedDisconnect)
        {
            if (client3)
                client3->process(5);
            if (server.getThreadCount() == 0)
                server.process(10);
        }
    }

    void processAllClients(Server& server)
    {
        int maxServiceLoops = 20;
        while (!(receivedMessage1 && receivedMessage2 && receivedReply1 &&
                 receivedReply2) &&
               --maxServiceLoops)
        {
            client1.process(5);
            client2.process(5);
            if (server.getThreadCount() == 0)
                server.process(10);
        }
    }
};
struct Fixture0 : public Fixture
{
    Server server{"", wsProtocol};
};
struct Fixture1 : public Fixture
{
    Server server{"", wsProtocol, 1u};
};
struct Fixture2 : public Fixture
{
    Server server{"", wsProtocol, 2u};
};
using Fixtures = boost::mpl::vector<Fixture0, Fixture1, Fixture2>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(client_send_text_message, F, Fixtures, F)
{
    F::server.handleText([&](const ws::Request& request) {
        F::receivedMessage1 = (request.message == "hello");
        return "server";
    });
    F::client1.handleText([&](const ws::Request& request) {
        F::receivedReply1 = (request.message == "server");
        return "";
    });

    connect(F::client1, F::server);
    BOOST_REQUIRE_EQUAL(F::server.getConnectionCount(), 1);

    F::client1.sendText("hello");
    F::processClient1(F::server);

    BOOST_CHECK(F::receivedMessage1);
    BOOST_CHECK(F::receivedReply1);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(client_send_binary_message, F, Fixtures, F)
{
    F::server.handleBinary([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "hello");
        F::receivedMessage1 = true;
        return "server";
    });
    F::client1.handleBinary([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "server");
        F::receivedReply1 = true;
        return "";
    });

    connect(F::client1, F::server);
    BOOST_REQUIRE_EQUAL(F::server.getConnectionCount(), 1);

    F::client1.sendBinary("hello", 5);
    F::processClient1(F::server);

    BOOST_CHECK(F::receivedMessage1);
    BOOST_CHECK(F::receivedReply1);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(client_send_large_binary_message, F, Fixtures,
                                 F)
{
    const std::string big(1024 * 1024 * 2, 'a');

    F::server.handleBinary([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message.size() == big.size());
        F::receivedMessage1 = true;
        return "server";
    });
    F::client1.handleBinary([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "server");
        F::receivedReply1 = true;
        return "";
    });

    connect(F::client1, F::server);
    BOOST_REQUIRE_EQUAL(F::server.getConnectionCount(), 1);

    F::client1.sendBinary(big.data(), big.size());
    F::processClient1(F::server);

    BOOST_CHECK(F::receivedMessage1);
    BOOST_CHECK(F::receivedReply1);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(server_broadcast_text_message, F, Fixtures, F)
{
    F::client1.handleText([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "hello");
        F::receivedMessage1 = true;
        return "client1";
    });
    F::client2.handleText([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "hello");
        F::receivedMessage2 = true;
        return "client2";
    });
    F::server.handleText([&](const ws::Request& request) {
        if (request.message == "client1")
            F::receivedReply1 = true;
        else if (request.message == "client2")
            F::receivedReply2 = true;
        else
            BOOST_REQUIRE(!"valid message");
        return "";
    });

    connect(F::client1, F::server);
    connect(F::client2, F::server);
    BOOST_REQUIRE_EQUAL(F::server.getConnectionCount(), 2);

    F::server.broadcastText("hello");
    F::processAllClients(F::server);

    BOOST_CHECK(F::receivedMessage1);
    BOOST_CHECK(F::receivedMessage2);
    BOOST_CHECK(F::receivedReply1);
    BOOST_CHECK(F::receivedReply2);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(server_broadcast_filtered_text_message, F,
                                 Fixtures, F)
{
    F::client1.handleText([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "hello");
        F::receivedMessage1 = true;
        return "client1";
    });
    F::client2.handleText([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "hello");
        F::receivedMessage2 = true;
        return "client2";
    });

    std::set<uintptr_t> filteredClient;
    F::server.handleOpen([&](const uintptr_t clientID) {
        filteredClient.insert(clientID);
        return std::vector<ws::Response>{{""}};
    });
    F::server.handleText([&](const ws::Request& request) {
        if (request.message == "client1")
            F::receivedReply1 = true;
        else if (request.message == "client2")
            F::receivedReply2 = true;
        else
            BOOST_REQUIRE(!"valid message");
        return "";
    });

    connect(F::client1, F::server);
    connect(F::client2, F::server);
    BOOST_REQUIRE_EQUAL(F::server.getConnectionCount(), 2);

    F::server.broadcastText("hello", filteredClient);
    F::processAllClients(F::server);

    BOOST_CHECK(!F::receivedMessage1);
    BOOST_CHECK(!F::receivedMessage2);
    BOOST_CHECK(!F::receivedReply1);
    BOOST_CHECK(!F::receivedReply2);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(server_broadcast_binary, F, Fixtures, F)
{
    F::client1.handleBinary([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "hello");
        F::receivedMessage1 = true;
        return "client1";
    });
    F::client2.handleBinary([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "hello");
        F::receivedMessage2 = true;
        return "client2";
    });
    F::server.handleBinary([&](const ws::Request& request) {
        if (request.message == "client1")
            F::receivedReply1 = true;
        else if (request.message == "client2")
            F::receivedReply2 = true;
        else
            BOOST_REQUIRE(!"valid message");
        return "";
    });

    connect(F::client1, F::server);
    connect(F::client2, F::server);
    BOOST_REQUIRE_EQUAL(F::server.getConnectionCount(), 2);

    F::server.broadcastBinary("hello", 5);
    F::processAllClients(F::server);

    BOOST_CHECK(F::receivedMessage1);
    BOOST_CHECK(F::receivedMessage2);
    BOOST_CHECK(F::receivedReply1);
    BOOST_CHECK(F::receivedReply2);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(server_send_text_response_to_others, F,
                                 Fixtures, F)
{
    F::client2.handleText([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "hello");
        F::receivedMessage2 = true;
        return "client2";
    });
    F::server.handleText([&](const ws::Request& request) {
        if (request.message == "client1")
        {
            F::receivedReply1 = true;
            return ws::Response{"hello", ws::Recipient::others};
        }
        else if (request.message == "client2")
            F::receivedReply2 = true;
        else
            BOOST_REQUIRE(!"valid message");
        return ws::Response{};
    });

    connect(F::client1, F::server);
    connect(F::client2, F::server);
    BOOST_REQUIRE_EQUAL(F::server.getConnectionCount(), 2);

    F::client1.sendText("client1");
    F::processAllClients(F::server);

    BOOST_CHECK(F::receivedMessage2);
    BOOST_CHECK(F::receivedReply2);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(server_send_binary_response_to_all, F,
                                 Fixtures, F)
{
    F::client1.handleBinary([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "bar");
        F::receivedMessage1 = true;
        return "";
    });
    F::client2.handleBinary([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "bar");
        F::receivedMessage2 = true;
        return "";
    });
    F::server.handleBinary([&](const ws::Request& request) {
        if (request.message == "foo")
            F::receivedReply1 = true;
        else
            BOOST_REQUIRE(!"valid message");
        return ws::Response{"bar", ws::Recipient::all};
    });

    connect(F::client1, F::server);
    connect(F::client2, F::server);
    connect(*F::client3, F::server);
    BOOST_REQUIRE_EQUAL(F::server.getConnectionCount(), 3);

    // makes sure to test that disconnected client is properly removed
    // (crashes otherwise)
    F::client3.reset();
    F::processAllClients(F::server);
    BOOST_REQUIRE_EQUAL(F::server.getConnectionCount(), 2);

    F::client1.sendBinary("foo", 3);
    F::processAllClients(F::server);

    BOOST_CHECK(F::receivedMessage1);
    BOOST_CHECK(F::receivedReply1);
    BOOST_CHECK(F::receivedMessage2);
    BOOST_CHECK(!F::receivedReply2);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(server_monitor_connections, F, Fixtures, F)
{
    short numConnections = 0;

    F::client3->handleText([&](const ws::Request& request) {
        BOOST_REQUIRE(request.message == "Connect");
        F::receivedConnectReply = true;
        return "";
    });

    F::server.handleOpen([&](const uintptr_t) {
        ++numConnections;
        F::receivedConnect = true;
        return std::vector<ws::Response>{
            {"Connect", ws::Recipient::sender, ws::Format::text}};
    });

    F::server.handleClose([&](const uintptr_t) {
        --numConnections;
        F::receivedDisconnect = true;
        return std::vector<ws::Response>{};
    });

    connect(*F::client3, F::server);
    BOOST_REQUIRE_EQUAL(F::server.getConnectionCount(), 1);

    F::processClient3Connect(F::server);
    BOOST_CHECK(F::receivedConnectReply);
    BOOST_CHECK_EQUAL(numConnections, 1);

    F::client3.reset();
    F::processClient3Disconnect(F::server);
    BOOST_CHECK_EQUAL(numConnections, 0);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(server_unspecified_format_response, F,
                                 Fixtures, F)
{
    F::server.handleOpen([&](const uintptr_t) {
        F::receivedConnect = true;
        return std::vector<ws::Response>{
            {"unspecified format", ws::Recipient::sender}};
    });

    F::client3->handleText([&](const ws::Request&) {
        F::receivedConnectReply = true;
        return "";
    });
    F::client3->handleBinary([&](const ws::Request&) {
        F::receivedConnectReply = true;
        return "";
    });

    connect(*F::client3, F::server);
    BOOST_CHECK(F::receivedConnect);
    BOOST_CHECK(!F::receivedConnectReply);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(text_binary_exchange_with_unhandled_binary, F,
                                 Fixtures, F)
{
    F::client1.handleText([&](const ws::Request& request) {
        BOOST_CHECK_EQUAL(request.message, "bla");
        return "";
    });

    connect(F::client1, F::server);

    F::server.broadcastText("bla");
    F::server.broadcastBinary("hello", 5);
    F::server.broadcastText("bla");
    F::processAllClients(F::server);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(text_binary_exchange_with_unhandled_text, F,
                                 Fixtures, F)
{
    F::client1.handleBinary([&](const ws::Request& request) {
        BOOST_CHECK_EQUAL(request.message, "hello");
        return "";
    });

    connect(F::client1, F::server);

    F::server.broadcastBinary("hello", 5);
    F::server.broadcastText("bla");
    F::server.broadcastBinary("hello", 5);
    F::processAllClients(F::server);
}
