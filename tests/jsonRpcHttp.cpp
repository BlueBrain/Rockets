/*********************************************************************/
/* Copyright (c) 2018, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#define BOOST_TEST_MODULE rockets_jsonrpc_http

#include <boost/test/unit_test.hpp>

#include "json_utils.h"

#include <rockets/http/client.h>
#include <rockets/http/utils.h>
#include <rockets/jsonrpc/client.h>
#include <rockets/jsonrpc/http.h>
#include <rockets/jsonrpc/receiver.h>

#include <iostream>

using namespace rockets;

namespace
{
jsonrpc::Response substractArr(const jsonrpc::Request& request)
{
    const auto array = nlohmann::json::parse(request.message);

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
    jsonrpc::Receiver receiver;

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

    using http::is_ready;
    auto maxTry = 100;
    while (--maxTry && (!is_ready(response1) || !is_ready(response2) ||
                        !is_ready(response3) || !is_ready(response4)))
    {
        httpClient.process(0);
        httpServer.process(0);
    }

    BOOST_REQUIRE(is_ready(response1));
    BOOST_CHECK_EQUAL(response1.get().result, "19");

    BOOST_REQUIRE(is_ready(response2));
    const auto error2 = response2.get();
    BOOST_CHECK_EQUAL(error2.result, "Invalid params");
    BOOST_CHECK_EQUAL(error2.error, -32602);

    BOOST_REQUIRE(is_ready(response3));
    BOOST_CHECK_EQUAL(response3.get().result, "-17");

    BOOST_REQUIRE(is_ready(response4));
    const auto error4 = response4.get();
    BOOST_CHECK_EQUAL(error4.result, "Invalid params");
    BOOST_CHECK_EQUAL(error4.error, -32602);
}
