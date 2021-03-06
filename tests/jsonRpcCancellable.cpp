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

#define BOOST_TEST_MODULE rockets_jsonrpc_cancellable

#include <boost/test/unit_test.hpp>

#include "rockets/json.hpp"
#include "rockets/jsonrpc/cancellableReceiver.h"

#include <iostream>
#include <thread>
namespace
{
const std::string action{
    R"({"jsonrpc": "2.0", "method": "action", "id": 4})"};

const std::string actionResponse{
    R"({
    "id": 4,
    "jsonrpc": "2.0",
    "result": 42
})"};

const std::string progressMessage{
    R"({
    "jsonrpc": "2.0",
    "method": "progress",
    "params": {
        "amount": 1.0,
        "id": 4,
        "operation": "update"
    }
})"};

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
    return {};
}

jsonrpc::CancelRequestCallback actionProgress(
    const jsonrpc::Request&, jsonrpc::AsyncResponse callback,
    jsonrpc::ProgressUpdateCallback progress)
{
    progress("update", 1.f);
    callback({std::to_string(42)});
    return {};
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
    std::string message;
    jsonrpc::CancellableReceiver jsonRpc{[& message = message](std::string msg,
                                                               uintptr_t){
        message = std::move(msg);
}
}
;
}
;

BOOST_FIXTURE_TEST_CASE(invalid_bind, Fixture)
{
    using namespace std::placeholders;
    BOOST_CHECK_THROW(jsonRpc.bindAsync("cancel",
                                        std::bind(&substractArrAsync, _1, _2)),
                      std::invalid_argument);
    BOOST_CHECK_THROW(jsonRpc.bind("cancel", std::bind(&substractArr, _1)),
                      std::invalid_argument);
    BOOST_CHECK_THROW(jsonRpc.bindAsync("progress",
                                        std::bind(&substractArrAsync, _1, _2)),
                      std::invalid_argument);
    BOOST_CHECK_THROW(jsonRpc.bind("progress", std::bind(&substractArr, _1)),
                      std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(process_arr_async_cancel, Fixture)
{
    using namespace std::placeholders;
    jsonRpc.bindAsync("subtract", std::bind(&substractArrAsyncForever, _1, _2));

    auto processResult = jsonRpc.processAsync(substractArray);
    std::atomic_bool done{false};
    std::thread waitForResult([&done, &processResult] {
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
    std::thread waitForResult([&done, &processResult] {
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
    std::thread waitForResult([&processResult] {
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

BOOST_FIXTURE_TEST_CASE(process_arr_async_progress, Fixture)
{
    using namespace std::placeholders;
    jsonRpc.bindAsync("action", std::bind(&actionProgress, _1, _2, _3));

    BOOST_CHECK_EQUAL(jsonRpc.processAsync(action).get(), actionResponse);
    BOOST_CHECK_EQUAL(message, progressMessage);
}
