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

#include "requestProcessor.h"
#include "utils.h"

#include <future>

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

const Response::Error parseError{"Parse error", ErrorCode::parse_error};
const Response::Error invalidParams{"Invalid params",
                                    ErrorCode::invalid_params};
const Response::Error invalidRequest{"Invalid Request",
                                     ErrorCode::invalid_request};
const Response::Error methodNotFound{"Method not found",
                                     ErrorCode::method_not_found};

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

} // anonymous namespace

void RequestProcessor::process(const Request& request,
                               AsyncStringResponse callback)
{
    try
    {
        const auto document = json::parse(request.message);
        if (document.is_object())
        {
            auto stringifyCallback = [callback](const json obj) {
                callback(dump(obj));
            };
            _processCommand(document, request.clientID, stringifyCallback);
        }
        else if (document.is_array())
            callback(_processBatchBlocking(document, request.clientID));
        else
            callback(dump(makeErrorResponse(invalidParams)));
    }
    catch (const json::parse_error& e)
    {
        callback(dump(makeErrorResponse(parseError, json(), e.what())));
    }
}

void RequestProcessor::verifyValidMethodName(const std::string& method) const
{
    if (begins_with(method, reservedMethodPrefix))
        throw std::invalid_argument(reservedMethodError);
}

std::string RequestProcessor::_processBatchBlocking(const json& array,
                                                    const uintptr_t clientID)
{
    if (array.empty())
        return "";
    return dump(_processValidBatchBlocking(array, clientID));
}

json RequestProcessor::_processValidBatchBlocking(const json& array,
                                                  const uintptr_t clientID)
{
    json responses;
    for (const auto& entry : array)
    {
        if (entry.is_object())
        {
            const auto response = _processCommandBlocking(entry, clientID);
            if (!response.is_null())
                responses.push_back(response);
        }
        else
            responses.push_back(makeErrorResponse(invalidRequest));
    }
    return responses;
}

json RequestProcessor::_processCommandBlocking(const json& request,
                                               const uintptr_t clientID)
{
    auto promise = std::make_shared<std::promise<json>>();
    auto future = promise->get_future();
    auto callback = [promise](json response) {
        promise->set_value(std::move(response));
    };
    _processCommand(request, clientID, callback);
    return future.get();
}

void RequestProcessor::_processCommand(const json& request,
                                       const uintptr_t clientID,
                                       JsonResponseCallback respond)
{
    const auto id = request.count("id") ? request["id"] : json();
    const bool isNotification = id.is_null();
    if (!_isValidJsonRpcRequest(request))
    {
        if (isNotification)
            respond(json());
        else
            respond(makeErrorResponse(invalidRequest, id));
        return;
    }

    const auto methodName = request["method"].get<std::string>();
    if (!isRegisteredMethodName(methodName))
    {
        if (isNotification)
            respond(json());
        else
            respond(makeErrorResponse(methodNotFound, id));
        return;
    }

    const auto params =
        request.find("params") == request.end() ? "" : dump(request["params"]);

    process(id, methodName, {params, clientID}, respond);
}
}
}
