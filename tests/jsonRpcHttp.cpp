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

#define BOOST_TEST_MODULE rockets_jsonrpc_http

#include <boost/test/unit_test.hpp>

#include "json_utils.h"

#include <rockets/helpers.h>
#include <rockets/http/client.h>
#include <rockets/jsonrpc/asyncReceiver.h>
#include <rockets/jsonrpc/client.h>
#include <rockets/jsonrpc/http.h>

#include <iostream>

using namespace rockets;

namespace
{
jsonrpc::Response substractArr(const jsonrpc::Request& request)
{
    const auto array = rockets_nlohmann::json::parse(request.message);

    if (array.size() != 2 || !array[0].is_number() || !array[1].is_number())
        return jsonrpc::Response::invalidParams();

    const auto value = array[0].get<int>() - array[1].get<int>();
    return {std::to_string(value)};
}
}

struct Fixture
{
    Fixture() { jsonrpc::connect(httpServer, "test/endpoint", receiver); }
    Server httpServer;
    jsonrpc::AsyncReceiver receiver;

    http::Client httpClient;
    jsonrpc::HttpCommunicator communicator{httpClient, httpServer.getURI() +
                                                           "/test/endpoint"};
    jsonrpc::Client<jsonrpc::HttpCommunicator> client{communicator};
};

BOOST_FIXTURE_TEST_CASE(json_rpc_over_http, Fixture)
{
    receiver.bind("substract", std::bind(&substractArr, std::placeholders::_1));

    auto response1 = client.request("substract", "[42, 23]");
    auto response2 = client.request("substract", "@#!4784");
    auto response3 = client.request("substract", "[25, 42]");
    auto response4 = client.request("substract", "[15]");

    auto maxTry = 1000;
    while (--maxTry && (!response1.is_ready() || !response2.is_ready() ||
                        !response3.is_ready() || !response4.is_ready()))
    {
        httpClient.process(10);
        httpServer.process(10);
    }

    BOOST_REQUIRE(response1.is_ready());
    BOOST_CHECK_EQUAL(response1.get().result, "19");

    BOOST_REQUIRE(response2.is_ready());
    const auto error2 = response2.get();
    BOOST_CHECK_EQUAL(error2.error.message, "Invalid params");
    BOOST_CHECK_EQUAL(error2.error.code, -32602);

    BOOST_REQUIRE(response3.is_ready());
    BOOST_CHECK_EQUAL(response3.get().result, "-17");

    BOOST_REQUIRE(response4.is_ready());
    const auto error4 = response4.get();
    BOOST_CHECK_EQUAL(error4.error.message, "Invalid params");
    BOOST_CHECK_EQUAL(error4.error.code, -32602);
}

BOOST_AUTO_TEST_CASE(abort_pending_request)
{
    auto clientStack = std::make_unique<Fixture>();
    auto response = clientStack->client.request("substract", "[42, 23]");
    clientStack.reset();

    BOOST_REQUIRE(response.is_ready());
    const auto error = response.get();
    BOOST_CHECK_EQUAL(error.error.message,
                      "Requester was destroyed before receiving a response");
    BOOST_CHECK_EQUAL(error.error.code, -31002);
}
