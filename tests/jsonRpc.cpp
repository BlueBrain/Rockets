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

#define BOOST_TEST_MODULE rockets_jsonrpc

#include <boost/test/unit_test.hpp>

#include "rockets/json.hpp"
#include "rockets/jsonrpc/receiver.h"

// Validation examples based on: http://www.jsonrpc.org/specification

namespace
{
const std::string substractArray{
    R"({"jsonrpc": "2.0", "method": "subtract", "params": [42, 23], "id": 3})"};

const std::string substractArrayStringId{
    R"({"jsonrpc": "2.0", "method": "subtract", "params": [42, 23], "id": "myId123"})"};

const std::string substractObject{
    R"({"jsonrpc": "2.0", "method": "subtract", "params": {"subtrahend": 23, "minuend": 42}, "id": 3})"};

const std::string substractNotification{
    R"({"jsonrpc": "2.0", "method": "subtract", "params": [42, 23]})"};

const std::string substractBatch{
    R"([{"jsonrpc": "2.0", "method": "subtract", "params": {"subtrahend": 23, "minuend": 42}, "id": 1},
        {"jsonrpc": "2.0", "method": "subtract", "params": {"subtrahend": 23, "minuend": 42}, "id": 3}])"};

const std::string substractBatchNotification{
    R"([{"jsonrpc": "2.0", "method": "subtract", "params": [42, 23]},
        {"jsonrpc": "2.0", "method": "subtract", "params": [42, 23]}])"};

const std::string substractBatchMixed{
    R"([{"jsonrpc": "2.0", "method": "subtract", "params": [42, 23], "id": 1},
        {"jsonrpc": "2.0", "method": "subtract", "params": [42, 23]},
        {"jsonrpc": "2.0", "method": "subtract", "params": [42, 23], "id": 3},
        {"jsonrpc": "2.0", "method": "subtract", "params": [42, 23]}])"};

const std::string invalidNotification{
    R"({"jsonrpc": "2.0", "method": 1, "params": "bar"})"};

const std::string invalidRequest{
    R"({"jsonrpc": "2.0", "method": 1, "params": "bar", "id": 6})"};

const std::string invalidJsonRpcVersionRequest{
    R"({"jsonrpc": "3.5", "method": "subtract", "params": [42, 23], "id": 3})"};

const std::string substractResult{
    R"({
    "id": 3,
    "jsonrpc": "2.0",
    "result": 19
})"};

const std::string substractResultStringId{
    R"({
    "id": "myId123",
    "jsonrpc": "2.0",
    "result": 19
})"};

const std::string connectStandardReply{
    R"({
    "id": 3,
    "jsonrpc": "2.0",
    "result": "OK"
})"};

const std::string substractBatchResult{
    R"([
    {
        "id": 1,
        "jsonrpc": "2.0",
        "result": 19
    },
    {
        "id": 3,
        "jsonrpc": "2.0",
        "result": 19
    }
])"};

const std::string nonExistantMethodResult{
    R"({
    "error": {
        "code": -32601,
        "message": "Method not found"
    },
    "id": 3,
    "jsonrpc": "2.0"
})"};

const std::string invalidJsonResult{
    R"({
    "error": {
        "code": -32700,
        "data": "[json.exception.parse_error.101] parse error at line 1, column 1: syntax error while parsing value - invalid literal; last read: 'Z'",
        "message": "Parse error"
    },
    "id": null,
    "jsonrpc": "2.0"
})"};

const std::string invalidJsonArrayResult{
    R"({
    "error": {
        "code": -32700,
        "data": "[json.exception.parse_error.101] parse error at line 1, column 10: syntax error while parsing array - unexpected ':'; expected ']'",
        "message": "Parse error"
    },
    "id": null,
    "jsonrpc": "2.0"
})"};

const std::string internalErrorInvalidJson{
    R"({
    "error": {
        "code": -32603,
        "data": "Server response is not a valid json string",
        "message": "Internal error"
    },
    "id": 3,
    "jsonrpc": "2.0"
})"};

const std::string invalidRequestResult{
    R"({
    "error": {
        "code": -32600,
        "message": "Invalid Request"
    },
    "id": 6,
    "jsonrpc": "2.0"
})"};

const std::string invalidJsonRpcVersionResult{
    R"({
    "error": {
        "code": -32600,
        "message": "Invalid Request"
    },
    "id": 3,
    "jsonrpc": "2.0"
})"};

const std::string invalidParams{
    R"({
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": null,
    "jsonrpc": "2.0"
})"};

const std::string invalidParamsResult{
    R"({
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": 3,
    "jsonrpc": "2.0"
})"};

const std::string customSubstrationErrorResult{
    R"({
    "error": {
        "code": -1234,
        "message": "No substractions today"
    },
    "id": 3,
    "jsonrpc": "2.0"
})"};

const std::string invalidBatch1RequestResult{
    R"([
    {
        "error": {
            "code": -32600,
            "message": "Invalid Request"
        },
        "id": null,
        "jsonrpc": "2.0"
    }
])"};

const std::string invalidBatch3RequestResult{
    R"([
    {
        "error": {
            "code": -32600,
            "message": "Invalid Request"
        },
        "id": null,
        "jsonrpc": "2.0"
    },
    {
        "error": {
            "code": -32600,
            "message": "Invalid Request"
        },
        "id": null,
        "jsonrpc": "2.0"
    },
    {
        "error": {
            "code": -32600,
            "message": "Invalid Request"
        },
        "id": null,
        "jsonrpc": "2.0"
    }
])"};
} // namespace

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
    jsonrpc::Receiver jsonRpc;
};

BOOST_FIXTURE_TEST_CASE(process_obj, Fixture)
{
    jsonRpc.bind("subtract", std::bind(&substractObj, std::placeholders::_1));
    BOOST_CHECK_EQUAL(jsonRpc.process(substractObject), substractResult);
}

BOOST_FIXTURE_TEST_CASE(process_arr, Fixture)
{
    jsonRpc.bind("subtract", std::bind(&substractArr, std::placeholders::_1));
    BOOST_CHECK_EQUAL(jsonRpc.process(substractArray), substractResult);
}

BOOST_FIXTURE_TEST_CASE(process_arr_with_string_id, Fixture)
{
    jsonRpc.bind("subtract", std::bind(&substractArr, std::placeholders::_1));
    BOOST_CHECK_EQUAL(jsonRpc.process(substractArrayStringId),
                      substractResultStringId);
}

BOOST_FIXTURE_TEST_CASE(process_notification, Fixture)
{
    // Notifications are rpc requests without an id.
    // They must be processed without returning a response.
    bool called = false;
    jsonrpc::Response response{""};
    jsonrpc::ResponseCallback action =
        [&called, &response](const jsonrpc::Request& request) {
            called = true;
            response = substractArr(request);
            return response;
        };
    jsonRpc.bind("subtract", action);
    BOOST_CHECK(jsonRpc.process(substractNotification).empty());
    BOOST_CHECK(called);
    BOOST_CHECK_EQUAL(response.result, "19");
}

BOOST_FIXTURE_TEST_CASE(process_unhandled_notification, Fixture)
{
    BOOST_CHECK(jsonRpc.process(substractNotification).empty());
}

BOOST_FIXTURE_TEST_CASE(bind_with_params, Fixture)
{
    jsonRpc.bind<Operands>("subtract", [](const Operands op) {
        return jsonrpc::Response{std::to_string(op.left - op.right)};
    });
    BOOST_CHECK_EQUAL(jsonRpc.process(substractObject), substractResult);
    BOOST_CHECK_EQUAL(jsonRpc.process(substractArray), invalidParamsResult);
}

BOOST_FIXTURE_TEST_CASE(bind_with_params_and_retval, Fixture)
{
    jsonRpc.bind<Operands, RetVal>("subtract", [](const Operands op) {
        return RetVal{op.left - op.right};
    });
    BOOST_CHECK_EQUAL(jsonRpc.process(substractObject), substractResult);
    BOOST_CHECK_EQUAL(jsonRpc.process(substractArray), invalidParamsResult);
}

BOOST_FIXTURE_TEST_CASE(bind_with_params_and_retval_error, Fixture)
{
    jsonRpc.bind<Operands, RetVal>("subtract", [](const Operands op) {
        if (op.right != op.left)
            throw jsonrpc::response_error("No substractions today", -1234);
        return RetVal();
    });
    BOOST_CHECK_EQUAL(jsonRpc.process(substractObject),
                      customSubstrationErrorResult);
}

BOOST_FIXTURE_TEST_CASE(connect_with_params, Fixture)
{
    int called = 0;
    jsonRpc.connect<Operands>("subtract", [&called](const Operands op) {
        BOOST_CHECK(op.left == 42 && op.right == 23);
        ++called;
    });
    BOOST_CHECK_EQUAL(jsonRpc.process(substractObject), connectStandardReply);
    BOOST_CHECK_EQUAL(jsonRpc.process(substractArray), invalidParamsResult);
    BOOST_CHECK_EQUAL(called, 1);
}

BOOST_FIXTURE_TEST_CASE(reserved_method_names, Fixture)
{
    using namespace std::placeholders;
    const auto bindFunc = std::bind(&substractObj, _1);
    ;

    BOOST_CHECK_THROW(jsonRpc.bind("rpc.xyz", bindFunc), std::invalid_argument);
    BOOST_CHECK_THROW(jsonRpc.connect("rpc.", [](const jsonrpc::Request&) {}),
                      std::invalid_argument);
    BOOST_CHECK_THROW(jsonRpc.connect("rpc.void", [] {}),
                      std::invalid_argument);

    BOOST_CHECK_NO_THROW(jsonRpc.bind("RPC.xyz", bindFunc));
    BOOST_CHECK_NO_THROW(jsonRpc.bind("rpc", bindFunc));
    BOOST_CHECK_NO_THROW(jsonRpc.bind("_rpc.", bindFunc));
}

BOOST_FIXTURE_TEST_CASE(non_existant_method, Fixture)
{
    BOOST_CHECK_EQUAL(jsonRpc.process(substractArray), nonExistantMethodResult);
}

BOOST_FIXTURE_TEST_CASE(callback_response_is_invalid_json, Fixture)
{
    jsonRpc.bind("subtract", [](const jsonrpc::Request&) {
        return jsonrpc::Response{"Not json!"};
    });
    BOOST_CHECK_EQUAL(jsonRpc.process(substractArray),
                      internalErrorInvalidJson);
}

BOOST_FIXTURE_TEST_CASE(invalid_json, Fixture)
{
    jsonRpc.bind("subtract", std::bind(&substractObj, std::placeholders::_1));
    BOOST_CHECK_EQUAL(jsonRpc.process({"Zorgy!!"}), invalidJsonResult);
}

BOOST_FIXTURE_TEST_CASE(invalid_json_rpc_version, Fixture)
{
    jsonRpc.bind("subtract", std::bind(&substractObj, std::placeholders::_1));
    BOOST_CHECK_EQUAL(jsonRpc.process(invalidJsonRpcVersionRequest),
                      invalidJsonRpcVersionResult);
}

BOOST_FIXTURE_TEST_CASE(wrong_json_rpc_notification, Fixture)
{
    jsonRpc.bind("subtract", std::bind(&substractObj, std::placeholders::_1));
    BOOST_CHECK_EQUAL(jsonRpc.process(invalidNotification), "");
}

BOOST_FIXTURE_TEST_CASE(wrong_json_rpc_request, Fixture)
{
    jsonRpc.bind("subtract", std::bind(&substractObj, std::placeholders::_1));
    BOOST_CHECK_EQUAL(jsonRpc.process(invalidRequest), invalidRequestResult);
}

BOOST_FIXTURE_TEST_CASE(invalid_array_requests, Fixture)
{
    jsonRpc.bind("subtract", std::bind(&substractObj, std::placeholders::_1));
    BOOST_CHECK_EQUAL(jsonRpc.process({"[\"Zorgy!\": 1]"}),
                      invalidJsonArrayResult);

    BOOST_CHECK_EQUAL(jsonRpc.process({"[]"}), "");
    BOOST_CHECK_EQUAL(jsonRpc.process({"[1]"}), invalidBatch1RequestResult);
    BOOST_CHECK_EQUAL(jsonRpc.process({"[1,2,3]"}), invalidBatch3RequestResult);
}

BOOST_FIXTURE_TEST_CASE(valid_array_request, Fixture)
{
    jsonRpc.bind("subtract", std::bind(&substractObj, std::placeholders::_1));
    BOOST_CHECK_EQUAL(jsonRpc.process(substractBatch), substractBatchResult);
}

BOOST_FIXTURE_TEST_CASE(invalid_params_request, Fixture)
{
    jsonRpc.bind("subtract", std::bind(&substractObj, std::placeholders::_1));
    BOOST_CHECK_EQUAL(jsonRpc.process({"5"}), invalidParams);
}

BOOST_FIXTURE_TEST_CASE(process_array_notification, Fixture)
{
    // Notifications are rpc requests without an id.
    // A batch of notification must be processed without returning a response.
    int called = 0;
    jsonrpc::Response response{""};
    jsonrpc::ResponseCallback action =
        [&called, &response](const jsonrpc::Request& request) {
            ++called;
            response = substractArr(request);
            return response;
        };
    jsonRpc.bind("subtract", action);
    BOOST_CHECK(jsonRpc.process(substractBatchNotification).empty());
    BOOST_CHECK_EQUAL(called, 2);
    BOOST_CHECK_EQUAL(response.result, "19");
}

BOOST_FIXTURE_TEST_CASE(process_array_mixed, Fixture)
{
    // For mixed notifications/requests batches, only respond to requests.
    int called = 0;
    jsonrpc::Response response{""};
    auto action = [&called, &response](const jsonrpc::Request& request) {
        ++called;
        response = substractArr(request);
        return response;
    };
    jsonRpc.bind("subtract", jsonrpc::ResponseCallback(action));
    BOOST_CHECK_EQUAL(jsonRpc.process(substractBatchMixed),
                      substractBatchResult);
    BOOST_CHECK_EQUAL(called, 4);
    BOOST_CHECK_EQUAL(response.result, "19");
}
