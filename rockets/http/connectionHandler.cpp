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

#include "connectionHandler.h"

#include "../helpers.h"
#include "helpers.h"
#include "request.h"
#include "response.h"

namespace
{
const std::string JSON_TYPE = "application/json";
const std::string REQUEST_REGISTRY = "registry";

const int codeContinue = 0;

std::string _removeEndpointFromPath(const std::string& endpoint,
                                    const std::string& path)
{
    if (endpoint == "/")
        return path;

    if (endpoint.size() >= path.size())
        return std::string();

    return path.substr(endpoint.size());
}
} // anonymous namespace

namespace rockets
{
namespace http
{
ConnectionHandler::ConnectionHandler(const Registry& registry)
    : _registry(registry)
{
}

void ConnectionHandler::setFilter(const Filter* filter)
{
    _filter = filter;
}

int ConnectionHandler::handleNewRequest(Connection& connection) const
{
    if (connection.isCorsPreflightRequest())
        return _replyToCorsPreflightRequest(connection);

    if (connection.canHaveHttpBody())
        return codeContinue;

    return respondToRequest(connection);
}

int ConnectionHandler::handleData(Connection& connection, const char* data,
                                  const size_t size) const
{
    connection.appendBody(data, size);
    return codeContinue;
}

int ConnectionHandler::respondToRequest(Connection& connection) const
{
    if (!connection.delayedResponseSet)
    {
        connection.delayedResponse = _generateResponse(connection);
        connection.delayedResponseSet = true;
    }

    Response response;
    try
    {
        if (!is_ready(connection.delayedResponse))
        {
            connection.delayResponse();
            return codeContinue;
        }
        response = connection.delayedResponse.get();
    }
    catch (const std::future_error&)
    {
        response = Response{Code::INTERNAL_SERVER_ERROR};
    }
    return connection.write(response, connection.getCorsResponseHeaders());
}

std::future<Response> ConnectionHandler::_generateResponse(
    Connection& connection) const
{
    auto& request = connection.getRequest();
    if (_filter && _filter->filter(request))
        return make_ready_response(_filter->getResponse(request));

    const auto path = connection.getPathWithoutLeadingSlash();

    if (connection.getMethod() == Method::GET && path == REQUEST_REGISTRY)
        return make_ready_response(Code::OK, _registry.toJson(), JSON_TYPE);

    const auto result = _registry.findEndpoint(connection.getMethod(), path);
    if (result.found)
    {
        const auto& endpoint = result.endpoint;
        const auto pathStripped = _removeEndpointFromPath(endpoint, path);
        if (pathStripped.empty() || *endpoint.rbegin() == '/')
        {
            request.path = pathStripped;
            return _callHandler(connection, endpoint);
        }
    }

    // return informative error 405 "Method Not Allowed" if possible
    const auto allowedMethods = _registry.getAllowedMethods(path);
    if (!allowedMethods.empty())
    {
        Response::Headers headers{{Header::ALLOW, allowedMethods}};
        return make_ready_response(Code::NOT_SUPPORTED, std::string(),
                                   std::move(headers));
    }

    return make_ready_response(Code::NOT_FOUND);
}

std::future<Response> ConnectionHandler::_callHandler(
    const Connection& connection, const std::string& endpoint) const
{
    auto func = _registry.getFunction(connection.getMethod(), endpoint);
    return func(connection.getRequest());
}

int ConnectionHandler::_replyToCorsPreflightRequest(
    Connection& connection) const
{
    // In a typical situation, user agents discover via a preflight request
    // whether a cross-origin resource is prepared to accept requests.
    // The current implementation accepts all sources for all requests.
    // More information can be found here: https://www.w3.org/TR/cors

    const auto path = connection.getPathWithoutLeadingSlash();
    const auto corsHeaders = _makeCorsPreflighResponseHeaders(path);
    return connection.write(Response{Code::OK}, corsHeaders);
}

CorsResponseHeaders ConnectionHandler::_makeCorsPreflighResponseHeaders(
    const std::string& path) const
{
    const auto allowedMethods = _registry.getAllowedMethods(path);
    return {{CorsResponseHeader::access_control_allow_headers, "Content-Type"},
            {CorsResponseHeader::access_control_allow_methods, allowedMethods},
            {CorsResponseHeader::access_control_allow_origin, "*"}};
}
}
}
