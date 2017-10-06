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

using namespace nlohmann;

namespace rockets
{
namespace jsonrpc
{
namespace
{
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
        return Response{error["message"].get<std::string>(),
                        error["code"].get<int>()};
    }
    return Response{object["result"].dump()};
}
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
    auto request = params.empty()
                       ? makeRequest(method, lastId)
                       : makeRequest(method, lastId, json::parse(params));
    pendingRequests.emplace(lastId++, callback);
    _sendRequest(request.dump(4));
}

bool Requester::processResponse(const std::string& json)
{
    const auto response = json::parse(json, nullptr, false);
    if (!isValidJsonRpcResponse(response))
        return false;

    const auto& id = response["id"];
    if (!id.is_number_unsigned())
        return false;

    auto it = pendingRequests.find(id.get<size_t>());
    if (it == pendingRequests.end())
        return false;

    it->second(makeResponse(response));
    pendingRequests.erase(it);
    return true;
}
}
}
