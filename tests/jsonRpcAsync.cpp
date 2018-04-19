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

#define BOOST_TEST_MODULE rockets_jsonrpc_async

#include <boost/test/unit_test.hpp>

#include "rockets/json.hpp"
#include "rockets/jsonrpc/asyncReceiver.h"

#include <thread>

// Validation examples based on: http://www.jsonrpc.org/specification

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

const std::string substractObject{
    R"({"jsonrpc": "2.0", "method": "subtract", "params": {"subtrahend": 23, "minuend": 42}, "id": 3})"};

const std::string invalidParamsResult{
    R"({
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": 3,
    "jsonrpc": "2.0"
})"};
}

using namespace rockets;

jsonrpc::Response substractObj(const jsonrpc::Request& request)
{
    const auto object = rockets_nlohmann::json::parse(request.message);

    if (!object.count("minuend") || !object["minuend"].is_number() ||
        !object.count("subtrahend") || !object["subtrahend"].is_number())
        return jsonrpc::Response::invalidParams();

    const auto value =
        object["minuend"].get<int>() - object["subtrahend"].get<int>();
    return {std::to_string(value)};
}

jsonrpc::Response substractArr(const jsonrpc::Request& request)
{
    const auto array = rockets_nlohmann::json::parse(request.message);

    if (array.size() != 2 || !array[0].is_number() || !array[1].is_number())
        return jsonrpc::Response::invalidParams();

    const auto value = array[0].get<int>() - array[1].get<int>();
    return {std::to_string(value)};
}

void substractArrAsync(const jsonrpc::Request& request,
                       jsonrpc::AsyncResponse callback)
{
    std::thread([request, callback]() {
        usleep(500);
        callback(substractArr(request));
    }).detach();
}

struct Operands
{
    int left = 0;
    int right = 0;
};
bool from_json(Operands& op, const std::string& json)
{
    const auto object = rockets_nlohmann::json::parse(json);

    if (!object.count("minuend") || !object["minuend"].is_number() ||
        !object.count("subtrahend") || !object["subtrahend"].is_number())
        return false;
    op.left = object["minuend"].get<int>();
    op.right = object["subtrahend"].get<int>();
    return true;
}

struct RetVal
{
    int value = 0;
};
std::string to_json(const RetVal& retVal)
{
    return std::to_string(retVal.value);
}

struct Fixture
{
    jsonrpc::AsyncReceiver jsonRpcAsync;
};

BOOST_FIXTURE_TEST_CASE(process_arr_async, Fixture)
{
    using namespace std::placeholders;
    jsonRpcAsync.bindAsync("subtract", std::bind(&substractArrAsync, _1, _2));
    BOOST_CHECK_EQUAL(jsonRpcAsync.processAsync(substractArray).get(),
                      substractResult);
}

BOOST_FIXTURE_TEST_CASE(bind_async_with_params, Fixture)
{
    jsonRpcAsync.bindAsync<Operands>(
        "subtract", [](const Operands op, jsonrpc::AsyncResponse callback) {
            callback(jsonrpc::Response{std::to_string(op.left - op.right)});
        });
    BOOST_CHECK_EQUAL(jsonRpcAsync.processAsync(substractObject).get(),
                      substractResult);
    BOOST_CHECK_EQUAL(jsonRpcAsync.processAsync(substractArray).get(),
                      invalidParamsResult);
}

BOOST_FIXTURE_TEST_CASE(reserved_method_names, Fixture)
{
    using namespace std::placeholders;
    const auto bindAsyncFunc = std::bind(&substractArrAsync, _1, _2);

    BOOST_CHECK_THROW(jsonRpcAsync.bindAsync("rpc.abc", bindAsyncFunc),
                      std::invalid_argument);
}
