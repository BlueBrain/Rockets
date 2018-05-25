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

#ifndef ROCKETS_HTTP_CONNECTION_H
#define ROCKETS_HTTP_CONNECTION_H

#include <rockets/http/channel.h>
#include <rockets/http/cors.h>
#include <rockets/http/request.h>
#include <rockets/http/types.h>

#include <libwebsockets.h>

namespace rockets
{
namespace http
{
/**
 * Incoming HTTP connection from a remote client on the Server.
 */
class Connection
{
public:
    Connection(lws* wsi, const char* path);

    // request

    std::string getPathWithoutLeadingSlash() const;
    Method getMethod() const;

    bool canHaveHttpBody() const;
    void appendBody(const char* in, const size_t len);

    bool isCorsPreflightRequest() const;

    const Request& getRequest() const { return request; }
    void overwriteRequestPath(std::string path);

    // response

    void setResponse(std::future<Response>&& futureResponse);
    void setCorsResponseHeaders(CorsResponseHeaders&& headers);

    bool isResponseSet() const;
    bool isResponseReady() const;

    void requestWriteCallback();

    int writeResponseHeaders();
    int writeResponseBody();

    bool wereResponseHeadersSent() const;

private:
    Channel channel;
    Request request;
    size_t contentLength = 0;
    CorsRequestHeaders corsHeaders;

    CorsResponseHeaders corsResponseHeaders;
    std::future<Response> delayedResponse;
    bool delayedResponseSet = false;
    bool responseFinalized = false;
    Response response;

    bool responseHeadersSent = false;
    bool responseBodySent = false;

    bool _canHaveHttpBody(Method m) const;
    bool _hasCorsPreflightHeaders() const;
    CorsResponseHeaders _getCorsResponseHeaders() const;
    void _finalizeResponse();
};
}
}

#endif
