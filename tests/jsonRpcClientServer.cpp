/*********************************************************************/
/* Copyright (c) 2017-2018, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

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
    auto future = client.request("test", simpleMessage);
    BOOST_CHECK(serverReceivedRequest);
    BOOST_REQUIRE(is_ready(future));

    const jsonrpc::Response response = future.get();
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
    auto future = client.request<std::string, int>("test", message);
    BOOST_CHECK(serverReceivedRequest);
    BOOST_REQUIRE(is_ready(future));
    BOOST_CHECK_EQUAL(future.get(), 42);
}

BOOST_FIXTURE_TEST_CASE(client_templated_request_with_no_parameter, Fixture)
{
    server.bind<int>("test", [&] { return 42; });
    auto future = client.request<int>("test");
    BOOST_REQUIRE(is_ready(future));
    BOOST_CHECK_EQUAL(future.get(), 42);
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
