/* Copyright (c) 2018, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
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

#include "http.h"

#include "../json.hpp"
#include "errorCodes.h"

using namespace rockets_nlohmann;

namespace rockets
{
namespace jsonrpc
{
namespace
{
std::string makeJsonHttpError(const std::string& errorMsg, const json& id)
{
    return json{{"jsonrpc", "2.0"},
                {"error",
                 json{{"message", errorMsg}, {"code", ErrorCode::http_error}}},
                {"id", id}}
        .dump(4);
}
}

void HttpCommunicator::sendText(std::string message)
{
    const auto id = json::parse(message)["id"];

    // Copying the callback in the lambda instead of *this* prevents potential
    // invalid memory access if the Communicator is destroyed before the
    // http::Client (which aborts pending requests, calling the error callback).
    const auto& cb = callback;
    client.request(url, http::Method::POST, std::move(message),
                   [cb](http::Response&& response) {
                       if (cb)
                           cb(ws::Request{std::move(response.body)});
                   },
                   [cb, id](std::string errorMsg) {
                       if (cb)
                           cb(ws::Request{makeJsonHttpError(errorMsg, id)});
                   });
}
}
}
