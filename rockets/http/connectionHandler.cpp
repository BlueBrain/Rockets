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

#include "connectionHandler.h"

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

void ConnectionHandler::handleNewRequest(Connection& connection) const
{
    if (connection.isCorsPreflightRequest())
        _prepareCorsPreflightResponse(connection);
    else if (!connection.canHaveHttpBody())
        prepareResponse(connection);
}

void ConnectionHandler::handleData(Connection& connection, const char* data,
                                   const size_t size) const
{
    connection.appendBody(data, size);
}

void ConnectionHandler::prepareResponse(Connection& connection) const
{
#if LWS_LIBRARY_VERSION_NUMBER >= 3001000
    // Since lws 3.1 LWS_CALLBACK_HTTP_BODY + LWS_CALLBACK_HTTP_BODY_COMPLETION
    // happen even when the POST request has ContentLength 0. Return to avoid a
    // logic exception because the response was already set in handleNewRequest.
    if (!connection.canHaveHttpBody() && connection.isResponseSet())
        return;
#endif
    connection.setResponse(_generateResponse(connection));
    connection.requestWriteCallback();
}

int ConnectionHandler::writeResponse(Connection& connection) const
{
    if (!connection.isResponseSet())
        throw std::logic_error("Response has not been prepared yet!");

    if (!connection.isResponseReady())
    {
        // Keep polling until response is ready
        connection.requestWriteCallback();
        return codeContinue;
    }

    if (!connection.wereResponseHeadersSent())
        return connection.writeResponseHeaders();

    return connection.writeResponseBody();
}

std::future<Response> ConnectionHandler::_generateResponse(
    Connection& connection) const
{
    const auto& request = connection.getRequest();
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
            connection.overwriteRequestPath(pathStripped);
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
    const auto& func = _registry.getFunction(connection.getMethod(), endpoint);
    return func(connection.getRequest());
}

void ConnectionHandler::_prepareCorsPreflightResponse(
    Connection& connection) const
{
    // In a typical situation, user agents discover via a preflight request
    // whether a cross-origin resource is prepared to accept requests.
    // The current implementation accepts all sources for all requests.
    // More information can be found here: https://www.w3.org/TR/cors

    const auto path = connection.getPathWithoutLeadingSlash();
    connection.setCorsResponseHeaders(_makeCorsPreflighResponseHeaders(path));
    connection.requestWriteCallback();
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
