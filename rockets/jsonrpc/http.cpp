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

using namespace nlohmann;

namespace
{
std::string makeJsonError(const std::string& errorMsg, const json& id)
{
    return json{{"jsonrpc", "2.0"},
                {"error", json{{"message", errorMsg}, {"code", -30100}}},
                {"id", id}}
        .dump(4);
}
}

namespace rockets
{
namespace jsonrpc
{
void HttpCommunicator::sendText(std::string message)
{
    const auto id = json::parse(message)["id"];
    client.request(url, http::Method::POST, std::move(message),
                   [this](http::Response response) {
                       if (callback)
                           callback(ws::Request{std::move(response.body)});
                   },
                   [this, id](std::string errorMsg) {
                       if (callback)
                           callback(ws::Request{makeJsonError(errorMsg, id)});
                   });
}
}
}
