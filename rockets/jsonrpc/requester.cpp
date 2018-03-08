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

#include "requester.h"

#include "../json.hpp"
#include "errorCodes.h"

using namespace rockets_nlohmann;

namespace rockets
{
namespace jsonrpc
{
namespace
{
const Response destructionError{
    Response::Error{"Requester was destroyed before receiving a response",
                    ErrorCode::request_aborted}};

json makeRequest(const std::string& method, const size_t id)
{
    return json{{"jsonrpc", "2.0"}, {"method", method}, {"id", id}};
}

json makeRequest(const std::string& method, const size_t id, json&& params)
{
    return json{{"jsonrpc", "2.0"},
                {"method", method},
                {"id", id},
                {"params", std::move(params)}};
}

bool isValidError(const json& error)
{
    return error.is_object() && error.count("code") &&
           error["code"].is_number_integer() && error.count("message") &&
           error["message"].is_string();
}

bool isValidId(const json& id)
{
    return id.is_number_integer() || id.is_string() || id.is_null();
}

bool isValidJsonRpcResponse(const json& object)
{
    return object.count("jsonrpc") &&
           object["jsonrpc"].get<std::string>() == "2.0" &&
           ((object.count("result") && !object.count("error")) ||
            (object.count("error") && !object.count("result") &&
             isValidError(object["error"]))) &&
           object.count("id") && isValidId(object["id"]);
}

Response makeResponse(const json& object)
{
    if (object.count("error"))
    {
        const auto& error = object["error"];
        return Response{Response::Error{error["message"].get<std::string>(),
                                        error["code"].get<int>()}};
    }
    return Response{object["result"].dump(4)};
}
}

class Requester::Impl
{
public:
    std::map<size_t, AsyncResponse> pendingRequests;
    size_t lastId = 0u;
};

Requester::Requester()
    : _impl{new Impl}
{
}

Requester::~Requester()
{
    for (auto&& it : _impl->pendingRequests)
        it.second(destructionError);
}

std::future<Response> Requester::request(const std::string& method,
                                         const std::string& params)
{
    auto promise = std::make_shared<std::promise<Response>>();
    auto callback = [promise](Response response) {
        promise->set_value(std::move(response));
    };
    request(method, params, callback);
    return promise->get_future();
}

void Requester::request(const std::string& method, const std::string& params,
                        AsyncResponse callback)
{
    try
    {
        const auto requestJSON =
            params.empty()
                ? makeRequest(method, _impl->lastId)
                : makeRequest(method, _impl->lastId, json::parse(params));

        _impl->pendingRequests.emplace(_impl->lastId++, callback);
        _send(requestJSON.dump(4));
    }
    catch (const json::parse_error&)
    {
        callback(Response::invalidParams());
    }
}

bool Requester::processResponse(const std::string& json)
{
    const auto response = json::parse(json, nullptr, false);
    if (!isValidJsonRpcResponse(response))
        return false;

    const auto& id = response["id"];
    if (!id.is_number_unsigned())
        return false;

    auto it = _impl->pendingRequests.find(id.get<size_t>());
    if (it == _impl->pendingRequests.end())
        return false;

    it->second(makeResponse(response));
    _impl->pendingRequests.erase(it);
    return true;
}

std::exception_ptr Requester::jsonConversionFailed()
{
    return std::make_exception_ptr(
        response_error("Response JSON conversion failed",
                       ErrorCode::invalid_json_response));
}
}
}
