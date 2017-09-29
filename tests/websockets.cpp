/* Copyright (c) 2017, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
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

#include <rockets/ws/client.h>
#include <rockets/server.h>

#include <iostream>

#include <boost/mpl/vector.hpp>
#include <boost/test/unit_test.hpp>

using namespace rockets;

namespace
{
const auto wsProtocol = "ws_test_protocol";

template <typename T>
bool is_ready(const std::future<T>& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void connect(ws::Client& client, Server& server)
{
    auto future = client.connect(server.getURI(), wsProtocol);
    while (!is_ready(future))
    {
        client.process(0);
        if (server.getThreadCount() == 0)
            server.process(0);
    }
    BOOST_REQUIRE_NO_THROW(future.get());
}
} // anonymous namespace

BOOST_AUTO_TEST_CASE(server_construction)
{
    Server server1{"", wsProtocol};
    BOOST_CHECK_NE(server1.getURI(), "");
    BOOST_CHECK_NE(server1.getPort(), 0);
    BOOST_CHECK_EQUAL(server1.getThreadCount(), 0);
}

BOOST_AUTO_TEST_CASE(client_connection_to_inexistant_server)
{
    ws::Client client;
    auto future = client.connect("inexistantServer:453", wsProtocol);
    BOOST_REQUIRE(is_ready(future));
    BOOST_CHECK_THROW(future.get(), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(client_connection_to_inexistant_protocol)
{
    Server server{"", wsProtocol};
    ws::Client client;
    auto future = client.connect(server.getURI(), "inexistantProtocol");
    BOOST_REQUIRE(!is_ready(future));
    while (!is_ready(future))
    {
        client.process(0);
        server.process(0);
    }
    BOOST_CHECK_THROW(future.get(), std::runtime_error);
}

/**
 * Fixtures to run all test cases with {0, 1, 2} server worker threads.
 */
struct Fixture
{
    ws::Client client1;
    ws::Client client2;
    std::atomic<bool> receivedMessage1 = {false};
    std::atomic<bool> receivedMessage2 = {false};
    std::atomic<bool> receivedReply1 = {false};
    std::atomic<bool> receivedReply2 = {false};

    void processClient1(Server& server)
    {
        int maxServiceLoops = 5;
        while (!(receivedMessage1 && receivedReply1) && --maxServiceLoops)
        {
            client1.process(5);
            if (server.getThreadCount() == 0)
                server.process(0);
        }
    }

    void processAllClients(Server& server)
    {
        int maxServiceLoops = 10;
        while (!(receivedMessage1 && receivedMessage2 && receivedReply1 &&
                 receivedReply2) && --maxServiceLoops)
        {
            client1.process(5);
            client2.process(5);
            if (server.getThreadCount() == 0)
                server.process(0);
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
    F::server.handleText([&](const std::string& message) {
        F::receivedMessage1 = (message == "hello");
        return "server";
    });
    F::client1.handleText([&](const std::string& message) {
        F::receivedReply1 = (message == "server");
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
    F::server.handleBinary([&](const std::string& message) {
        BOOST_REQUIRE(message == "hello");
        F::receivedMessage1 = true;
        return "server";
    });
    F::client1.handleBinary([&](const std::string& message) {
        BOOST_REQUIRE(message == "server");
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

BOOST_FIXTURE_TEST_CASE_TEMPLATE(server_broadcast_text_message, F, Fixtures, F)
{
    F::client1.handleText([&](const std::string& message) {
        BOOST_REQUIRE(message == "hello");
        F::receivedMessage1 = true;
        return "client1";
    });
    F::client2.handleText([&](const std::string& message) {
        BOOST_REQUIRE(message == "hello");
        F::receivedMessage2 = true;
        return "client2";
    });
    F::server.handleText([&](const std::string& message)
    {
        if (message == "client1")
            F::receivedReply1 = true;
        else if(message == "client2")
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

BOOST_FIXTURE_TEST_CASE_TEMPLATE(server_broadcast_binary, F, Fixtures, F)
{
    F::client1.handleBinary([&](const std::string& message) {
        BOOST_REQUIRE(message == "hello");
        F::receivedMessage1 = true;
        return "client1";
    });
    F::client2.handleBinary([&](const std::string& message) {
        BOOST_REQUIRE(message == "hello");
        F::receivedMessage2 = true;
        return "client2";
    });
    F::server.handleBinary([&](const std::string& message)
    {
        if (message == "client1")
            F::receivedReply1 = true;
        else if(message == "client2")
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
