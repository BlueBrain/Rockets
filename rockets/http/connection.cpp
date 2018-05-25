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

#include "connection.h"

#include "../helpers.h"
#include "utils.h"

namespace
{
const std::logic_error response_already_set_error{"response was already set!"};
const std::logic_error headers_already_sent_error{
    "response headers were already sent!"};
const std::logic_error headers_not_sent_error{
    "response headers have not been sent yet!"};
const std::logic_error body_already_sent_error{
    "response body has already been sent!"};
const std::logic_error body_empty_error{"response body is empty!"};
}

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
    , corsResponseHeaders(_getCorsResponseHeaders())
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

void Connection::overwriteRequestPath(std::string path)
{
    request.path = std::move(path);
}

void Connection::setResponse(std::future<Response>&& futureResponse)
{
    if (isResponseSet())
        throw response_already_set_error;

    delayedResponse = std::move(futureResponse);
    delayedResponseSet = true;
}

void Connection::setCorsResponseHeaders(CorsResponseHeaders&& headers)
{
    if (isResponseSet())
        throw response_already_set_error;

    corsResponseHeaders = std::move(headers);
    responseFinalized = true;
}

bool Connection::isResponseSet() const
{
    return delayedResponseSet || responseFinalized;
}

bool Connection::isResponseReady() const
{
    return responseFinalized || is_ready(delayedResponse);
}

void Connection::requestWriteCallback()
{
    channel.requestCallback();
}

int Connection::writeResponseHeaders()
{
    if (responseHeadersSent)
        throw headers_already_sent_error;
    if (responseBodySent)
        throw body_already_sent_error;

    if (!responseFinalized)
        _finalizeResponse();

    responseHeadersSent = true;
    return channel.writeResponseHeaders(corsResponseHeaders, response);
}

int Connection::writeResponseBody()
{
    if (!responseHeadersSent)
        throw headers_not_sent_error;
    if (responseBodySent)
        throw body_already_sent_error;
    if (response.body.empty())
        throw body_empty_error;

    responseBodySent = true;
    return channel.writeResponseBody(response);
}

bool Connection::wereResponseHeadersSent() const
{
    return responseHeadersSent;
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

CorsResponseHeaders Connection::_getCorsResponseHeaders() const
{
    if (corsHeaders.origin.empty())
        return CorsResponseHeaders();
    return {{CorsResponseHeader::access_control_allow_origin, "*"}};
}

void Connection::_finalizeResponse()
{
    try
    {
        response = delayedResponse.get();
    }
    catch (const std::future_error&)
    {
        response = Response{Code::INTERNAL_SERVER_ERROR};
    }
    responseFinalized = true;
}
}
}
