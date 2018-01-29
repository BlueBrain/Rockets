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

#include "receiver.h"

#include "../json.hpp"

#include <map>

using namespace nlohmann;

namespace rockets
{
namespace jsonrpc
{
namespace
{
const std::string reservedMethodPrefix = "rpc.";
const char* reservedMethodError =
    "Method names starting with 'rpc.' are "
    "reserved by the standard / forbidden.";

const json parseError{{"code", -32700}, {"message", "Parse error"}};
const json invalidRequest{{"code", -32600}, {"message", "Invalid Request"}};
const json methodNotFound{{"code", -32601}, {"message", "Method not found"}};

json makeResponse(const std::string& result, const json& id)
{
    return json{{"jsonrpc", "2.0"},
                {"result", json::parse(result)},
                {"id", id}};
}

json makeErrorResponse(const json& error, const json& id = json())
{
    return json{{"jsonrpc", "2.0"}, {"error", error}, {"id", id}};
}

json makeErrorResponse(const int code, const std::string& message,
                       const json& id)
{
    return makeErrorResponse(json{{"code", code}, {"message", message}}, id);
}

bool _isValidJsonRpcRequest(const json& object)
{
    return object.count("jsonrpc") &&
           object["jsonrpc"].get<std::string>() == "2.0" &&
           object.count("method") && object["method"].is_string() &&
           (!object.count("params") || object["params"].is_object() ||
            object["params"].is_array()) &&
           (!object.count("id") || object["id"].is_number() ||
            object["id"].is_string());
}

inline std::string dump(const json& object)
{
    return object.is_null() ? "" : object.dump(4);
}

inline bool begins_with(const std::string& string, const std::string& other)
{
    return string.compare(0, other.length(), other) == 0;
}
} // anonymous namespace

class Receiver::Impl
{
public:
    std::string processBatchBlocking(const json& array,
                                     const uintptr_t clientID)
    {
        if (array.empty())
            return "";
        return dump(processValidBatchBlocking(array, clientID));
    }

    json processValidBatchBlocking(const json& array, const uintptr_t clientID)
    {
        json responses;
        for (const auto& entry : array)
        {
            if (entry.is_object())
            {
                const auto response = processCommandBlocking(entry, clientID);
                if (!response.is_null())
                    responses.push_back(response);
            }
            else
                responses.push_back(makeErrorResponse(invalidRequest));
        }
        return responses;
    }

    json processCommandBlocking(const json& request, const uintptr_t clientID)
    {
        auto promise = std::make_shared<std::promise<json>>();
        auto future = promise->get_future();
        auto callback = [promise](json response) {
            promise->set_value(std::move(response));
        };
        processCommand(request, clientID, callback);
        return future.get();
    }

    void processCommand(const json& request, const uintptr_t clientID,
                        std::function<void(json)> callback)
    {
        const auto id = request.count("id") ? request["id"] : json();
        const bool isNotification = id.is_null();
        if (!_isValidJsonRpcRequest(request))
        {
            if (isNotification)
                callback(json());
            else
                callback(makeErrorResponse(invalidRequest, id));
            return;
        }

        const auto methodName = request["method"].get<std::string>();
        const auto method = methods.find(methodName);
        if (method == methods.end())
        {
            if (isNotification)
                callback(json());
            else
                callback(makeErrorResponse(methodNotFound, id));
            return;
        }

        const auto params = request.find("params") == request.end()
                                ? ""
                                : dump(request["params"]);
        const auto& func = method->second;
        func({params, clientID},
             [callback, id](const Response rep) {
                 // No reply for valid "notifications" (requests without an
                 // "id")
                 if (id.is_null())
                 {
                     callback(json());
                     return;
                 }

                 if (rep.error != 0)
                     callback(makeErrorResponse(rep.error, rep.result, id));
                 else
                     callback(makeResponse(rep.result, id));
             });
    }
    std::map<std::string, Receiver::DelayedResponseCallback> methods;
};

Receiver::Receiver()
    : _impl{new Impl}
{
}

Receiver::~Receiver()
{
}

void Receiver::connect(const std::string& method, VoidCallback action)
{
    bind(method, [action](const Request&) {
        action();
        return Response{"\"OK\""};
    });
}

void Receiver::connect(const std::string& method, NotifyCallback action)
{
    bind(method, [action](const Request& request) {
        action(request);
        return Response{"\"OK\""};
    });
}

void Receiver::bind(const std::string& method, ResponseCallback action)
{
    bindAsync(method,
              [this, action](const Request& req, AsyncResponse callback) {
                  callback(action(req));
              });
}

void Receiver::bindAsync(const std::string& method,
                         DelayedResponseCallback action)
{
    if (begins_with(method, reservedMethodPrefix))
        throw std::invalid_argument(reservedMethodError);

    _impl->methods[method] = action;
}

std::string Receiver::process(const Request& request)
{
    return processAsync(request).get();
}

std::future<std::string> Receiver::processAsync(const Request& request)
{
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();
    auto callback = [promise](std::string response) {
        promise->set_value(std::move(response));
    };
    process(request, callback);
    return future;
}

void Receiver::process(const Request& request, AsyncStringResponse callback)
{
    const auto document = json::parse(request.message, nullptr, false);
    if (document.is_object())
    {
        auto stringifyCallback = [callback](const json obj) {
            callback(dump(obj));
        };
        _impl->processCommand(document, request.clientID, stringifyCallback);
    }
    else if (document.is_array())
        callback(_impl->processBatchBlocking(document, request.clientID));
    else
        callback(dump(makeErrorResponse(parseError)));
}
}
}
