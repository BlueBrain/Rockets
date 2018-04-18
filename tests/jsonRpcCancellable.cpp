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

#define BOOST_TEST_MODULE rockets_jsonRpcCancellable

#include <boost/test/unit_test.hpp>

#include "rockets/json.hpp"
#include "rockets/jsonrpc/cancellableReceiver.h"

#include <iostream>
#include <thread>
namespace
{
const std::string substractArray{
    R"({"jsonrpc": "2.0", "method": "subtract", "params": [42, 23], "id": 3})"};

const std::string substractResult{
    R"({
    "id": 3,
    "jsonrpc": "2.0",
    "result": 19
})"};

const std::string cancelSubstractArray{
    R"({"jsonrpc": "2.0", "method": "cancel", "params": { "id": 3 }})"};

const std::string invalidCancelNoParam{
    R"({"jsonrpc": "2.0", "method": "cancel"})"};

const std::string invalidCancelNoID{
    R"({"jsonrpc": "2.0", "method": "cancel", "params": { "foo": 3 }})"};

const std::string cancelledRequestResult{
    R"({
    "error": {
        "code": -31002,
        "message": "Request aborted"
    },
    "id": 3,
    "jsonrpc": "2.0"
})"};
}

using namespace rockets;
jsonrpc::Response substractArr(const jsonrpc::Request& request)
{
    const auto array = rockets_nlohmann::json::parse(request.message);

    if (array.size() != 2 || !array[0].is_number() || !array[1].is_number())
        return jsonrpc::Response::invalidParams();

    const auto value = array[0].get<int>() - array[1].get<int>();
    return {std::to_string(value)};
}

jsonrpc::CancelRequestCallback substractArrAsync(
    const jsonrpc::Request& request, jsonrpc::AsyncResponse callback)
{
    std::thread([request, callback]() {
        usleep(500);
        callback(substractArr(request));
    }).detach();
    return [](jsonrpc::VoidCallback done) { done(); };
}

std::mutex forever;

jsonrpc::CancelRequestCallback substractArrAsyncForever(const jsonrpc::Request&,
                                                        jsonrpc::AsyncResponse)
{
    std::thread([&]() { forever.lock(); }).detach();
    return [&](jsonrpc::VoidCallback done) {
        forever.unlock();
        done();
    };
}

void cancelTooLongTakingFunction()
{
    forever.unlock();
}

void cancelNOP()
{
}

struct Fixture
{
    jsonrpc::CancellableReceiver jsonRpc;
};

BOOST_FIXTURE_TEST_CASE(invalid_bind, Fixture)
{
    using namespace std::placeholders;
    BOOST_CHECK_THROW(jsonRpc.bindAsync("cancel",
                                        std::bind(&substractArrAsync, _1, _2)),
                      std::invalid_argument);
    BOOST_CHECK_THROW(jsonRpc.bind("cancel", std::bind(&substractArr, _1)),
                      std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(process_arr_async_cancel, Fixture)
{
    using namespace std::placeholders;
    jsonRpc.bindAsync("subtract", std::bind(&substractArrAsyncForever, _1, _2));

    auto processResult = jsonRpc.processAsync(substractArray);
    std::atomic_bool done{false};
    std::thread waitForResult([this, &done, &processResult] {
        BOOST_CHECK_EQUAL(processResult.get(), cancelledRequestResult);
        done = true;
    });

    while (!done)
    {
        usleep(500);
        BOOST_CHECK(jsonRpc.processAsync(cancelSubstractArray).get().empty());
    }

    waitForResult.join();
}

BOOST_FIXTURE_TEST_CASE(process_invalid_cancel_message, Fixture)
{
    using namespace std::placeholders;
    jsonRpc.bindAsync("subtract", std::bind(&substractArrAsyncForever, _1, _2));

    auto processResult = jsonRpc.processAsync(substractArray);
    std::atomic_bool done{false};
    std::thread waitForResult([this, &done, &processResult] {
        BOOST_CHECK_EQUAL(processResult.get(), cancelledRequestResult);
        done = true;
    });

    BOOST_CHECK(jsonRpc.processAsync(invalidCancelNoParam).get().empty());
    BOOST_CHECK(!done);

    BOOST_CHECK(jsonRpc.processAsync(invalidCancelNoID).get().empty());
    BOOST_CHECK(!done);

    while (!done)
        BOOST_CHECK(jsonRpc.processAsync(cancelSubstractArray).get().empty());

    waitForResult.join();
}

BOOST_FIXTURE_TEST_CASE(process_arr_async_cancel_but_also_finished, Fixture)
{
    using namespace std::placeholders;
    jsonRpc.bindAsync("subtract", std::bind(&substractArrAsyncForever, _1, _2));

    auto processResult = jsonRpc.processAsync(substractArray);
    std::thread waitForResult([this, &processResult] {
        BOOST_CHECK_EQUAL(processResult.get(), cancelledRequestResult);
    });

    BOOST_CHECK(jsonRpc.processAsync(cancelSubstractArray).get().empty());
    forever.unlock();

    waitForResult.join();
}

BOOST_FIXTURE_TEST_CASE(process_arr_async_cancel_already_finished, Fixture)
{
    using namespace std::placeholders;
    jsonRpc.bindAsync("subtract", std::bind(&substractArrAsync, _1, _2));

    BOOST_CHECK_EQUAL(jsonRpc.processAsync(substractArray).get(),
                      substractResult);

    BOOST_CHECK(jsonRpc.processAsync(cancelSubstractArray).get().empty());
}
