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

#include "emitter.h"

#include "../json.hpp"

using namespace nlohmann;

namespace rockets
{
namespace jsonrpc
{
namespace
{
json makeNotification(const std::string& method)
{
    return json{{"jsonrpc", "2.0"}, {"method", method}};
}

json makeNotification(const std::string& method, json&& params)
{
    return json{{"jsonrpc", "2.0"},
                {"method", method},
                {"params", std::move(params)}};
}
}

void Emitter::emit(const std::string& method, const std::string& params)
{
    const auto notification =
        params.empty() ? makeNotification(method)
                       : makeNotification(method, json::parse(params));
    _sendNotification(notification.dump(4));
}
}
}
