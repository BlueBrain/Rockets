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

#define BOOST_TEST_MODULE rockets_jsonrpc_client_server

#include <boost/test/unit_test.hpp>

#include "json_utils.h"

template <typename T>
std::string to_json(const T& obj)
{
    rockets_nlohmann::json json;
    json["value"] = obj;
    return json.dump(4);
}
template <typename T>
bool from_json(T& obj, const std::string& json)
{
    obj = rockets_nlohmann::json::parse(json)["value"];
    return true;
}

#include <rockets/helpers.h>
#include <rockets/jsonrpc/client.h>
#include <rockets/jsonrpc/server.h>
#include <rockets/server.h>
#include <rockets/ws/client.h>

using namespace rockets;

namespace
{
const std::string simpleMessage = json_reformat(R"({ "value": true })");
}

struct MockServerCommunicator
{
    void handleText(ws::MessageCallbackAsync callback)
    {
        handleMessageAsync = callback;
    }

    void sendText(std::string, uintptr_t) {}
    void sendText(std::string message)
    {
        auto ret = sendToRemoteEndpoint({message});
        if (!ret.message.empty())
            handleMessageAsync(std::move(ret.message), [](std::string) {});
    }

    void broadcastText(const std::string& message)
    {
        sendToRemoteEndpoint({message});
    }

    ws::MessageCallbackAsync handleMessageAsync;
    ws::MessageCallback sendToRemoteEndpoint;
};

struct MockClientCommunicator
{
    void handleText(ws::MessageCallback callback)
    {
        handleMessage = std::move(callback);
    }

    void sendText(std::string, uintptr_t) {}
    void sendText(std::string message)
    {
        auto handleResponse = [this](std::string ret) {
            if (!ret.empty())
            {
                handleMessage(std::move(ret));
                receivedMessage = true;
            }
        };
        sendToRemoteEndpoint(message, handleResponse);
    }

    void connectWith(MockServerCommunicator& other)
    {
        sendToRemoteEndpoint = other.handleMessageAsync;
        other.sendToRemoteEndpoint = handleMessage;
    }

    ws::MessageCallback handleMessage;
    ws::MessageCallbackAsync sendToRemoteEndpoint;
    bool receivedMessage = false;
};

BOOST_AUTO_TEST_CASE(client_constructor)
{
    ws::Client wsClient;
    jsonrpc::Client<ws::Client> client{wsClient};
}

BOOST_AUTO_TEST_CASE(server_constructor)
{
    Server wsServer;
    jsonrpc::Server<Server> server{wsServer};
}

BOOST_AUTO_TEST_CASE(close_client_while_request_pending)
{
    Server wsServer("", "test");
    jsonrpc::Server<Server> server{wsServer};

    auto wsClient = std::make_unique<ws::Client>();
    auto client = std::make_unique<jsonrpc::Client<ws::Client>>(*wsClient);

    auto connectFuture = wsClient->connect(wsServer.getURI(), "test");
    while (!is_ready(connectFuture))
    {
        wsClient->process(5);
        wsServer.process(5);
    }
    connectFuture.get();

    std::mutex forever;
    std::atomic_bool responseSent{false};
    server.bindAsync("forever",
                     [&forever,
                      &responseSent](const jsonrpc::Request&,
                                     jsonrpc::AsyncResponse response,
                                     jsonrpc::ProgressUpdateCallback) {
                         std::thread([&forever, &responseSent, response] {
                             forever.lock();
                             response({"false"});
                             responseSent = true;
                         }).detach();
                         return jsonrpc::CancelRequestCallback();
                     });
    auto request = client->request<bool>("forever");
    BOOST_REQUIRE(!request.is_ready());

    wsClient->process(5);
    wsServer.process(5);

    client.reset();
    wsClient.reset();

    forever.unlock();
    while (!responseSent)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    BOOST_CHECK(request.is_ready());
    BOOST_CHECK_THROW(request.get(), std::runtime_error);
}

struct Fixture
{
    MockServerCommunicator serverCommunicator;
    MockClientCommunicator clientCommunicator;
    jsonrpc::Server<MockServerCommunicator> server{serverCommunicator};
    jsonrpc::Client<MockClientCommunicator> client{clientCommunicator};
    Fixture() { clientCommunicator.connectWith(serverCommunicator); }
};

BOOST_FIXTURE_TEST_CASE(client_notification_received_by_server, Fixture)
{
    bool received = false;
    server.connect("test", [&](const jsonrpc::Request& request) {
        received = (request.message == simpleMessage);
    });
    client.notify("test", simpleMessage);
    BOOST_CHECK(received);
}

BOOST_FIXTURE_TEST_CASE(client_request_answered_by_server, Fixture)
{
    bool serverReceivedRequest = false;
    bool clientReceivedReply = false;
    std::string receivedValue;
    server.bind("test", [&](const jsonrpc::Request& request) {
        serverReceivedRequest = (request.message == simpleMessage);
        return jsonrpc::Response{"\"42\""};
    });
    client.request("test", simpleMessage, [&](jsonrpc::Response response) {
        clientReceivedReply = !response.isError();
        receivedValue = response.result;
    });
    BOOST_CHECK(serverReceivedRequest);
    BOOST_CHECK(clientReceivedReply);
    BOOST_CHECK_EQUAL(receivedValue, "\"42\"");
}

BOOST_FIXTURE_TEST_CASE(client_request_answered_by_server_using_future, Fixture)
{
    bool serverReceivedRequest = false;
    server.bind("test", [&](const jsonrpc::Request&& request) {
        serverReceivedRequest = (request.message == simpleMessage);
        return jsonrpc::Response{"\"42\""};
    });
    auto request = client.request("test", simpleMessage);
    BOOST_CHECK(serverReceivedRequest);
    BOOST_REQUIRE(request.is_ready());

    const jsonrpc::Response response = request.get();
    BOOST_CHECK(!response.isError());
    BOOST_CHECK_EQUAL(response.result, "\"42\"");
}

BOOST_FIXTURE_TEST_CASE(client_templated_request_answered_by_server, Fixture)
{
    bool serverReceivedRequest = false;
    const std::string message{"give me 42"};
    server.bind<std::string, int>("test", [&](const std::string& request) {
        serverReceivedRequest = (request == message);
        return 42;
    });
    auto request = client.request<std::string, int>("test", message);
    BOOST_CHECK(serverReceivedRequest);
    BOOST_REQUIRE(request.is_ready());
    BOOST_CHECK_EQUAL(request.get(), 42);
}

BOOST_FIXTURE_TEST_CASE(client_templated_request_with_no_parameter, Fixture)
{
    server.bind<int>("test", [&] { return 42; });
    auto request = client.request<int>("test");
    BOOST_REQUIRE(request.is_ready());
    BOOST_CHECK_EQUAL(request.get(), 42);
}

BOOST_FIXTURE_TEST_CASE(client_notification_generates_no_response, Fixture)
{
    bool serverReceivedRequest = false;
    server.bind("test", [&](const jsonrpc::Request&& request) {
        serverReceivedRequest = (request.message == simpleMessage);
        return jsonrpc::Response{"42"};
    });
    client.notify("test", simpleMessage);
    BOOST_CHECK(serverReceivedRequest);
    BOOST_CHECK(!clientCommunicator.receivedMessage);
}

BOOST_FIXTURE_TEST_CASE(server_notification_received_by_client, Fixture)
{
    bool received = false;
    client.connect("test", [&](const jsonrpc::Request&& request) {
        received = (request.message == simpleMessage);
    });
    server.notify("test", simpleMessage);
    BOOST_CHECK(received);
}

BOOST_FIXTURE_TEST_CASE(client_cancel_request, Fixture)
{
    using namespace std::placeholders;

    std::mutex forever;
    server.bindAsync("forever",
                     [&forever](const jsonrpc::Request&, jsonrpc::AsyncResponse,
                                jsonrpc::ProgressUpdateCallback) {
                         std::thread([&]() { forever.lock(); }).detach();
                         return [&](jsonrpc::VoidCallback done) {
                             forever.unlock();
                             done();
                         };
                     });
    auto request = client.request<bool>("forever");
    BOOST_REQUIRE(!request.is_ready());
    request.cancel();
    BOOST_CHECK(request.is_ready());
    BOOST_CHECK_THROW(request.get(), std::runtime_error);
}
