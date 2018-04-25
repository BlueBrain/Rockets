/* Copyright (c) 2017, EPFL/Blue Brain Project
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

#include "connection.h"

#include "utils.h"

namespace rockets
{
namespace http
{
Connection::Connection(lws* wsi, const char* path)
    : channel{wsi}
    , request{channel.readMethod(), path, channel.readOrigin(),
              channel.readQueryParameters(), ""}
    , contentLength{channel.readContentLength()}
    , corsHeaders(channel.readCorsRequestHeaders())
{
    request.body.reserve(contentLength);
}

std::string Connection::getPathWithoutLeadingSlash() const
{
    const auto& path = request.path;
    return (!path.empty() && path[0] == '/') ? path.substr(1) : path;
}

Method Connection::getMethod() const
{
    return request.method;
}

bool Connection::canHaveHttpBody() const
{
    return _canHaveHttpBody(getMethod()) && contentLength > 0;
}

void Connection::appendBody(const char* in, const size_t len)
{
    request.body.append(in, len);
}

bool Connection::isCorsPreflightRequest() const
{
    return getMethod() == Method::OPTIONS && _hasCorsPreflightHeaders();
}

CorsResponseHeaders Connection::getCorsResponseHeaders() const
{
    if (corsHeaders.origin.empty())
        return CorsResponseHeaders();
    return {{CorsResponseHeader::access_control_allow_origin, "*"}};
}

void Connection::delayResponse()
{
    channel.requestCallback();
}

int Connection::write(const Response& response, const CorsResponseHeaders& cors)
{
    return channel.write(response, cors);
}

bool Connection::_canHaveHttpBody(const Method m) const
{
    return m == Method::POST || m == Method::PUT || m == Method::PATCH;
}

bool Connection::_hasCorsPreflightHeaders() const
{
    return !corsHeaders.accessControlRequestHeaders.empty() &&
           // requires parsing Access-Control-Request-Method in libwebsockets:
           // corsHeaders.accessControlRequestMethod != Method::ALL &&
           !corsHeaders.origin.empty();
}
}
}
