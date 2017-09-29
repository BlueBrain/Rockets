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

    std::string getPathWithoutLeadingSlash() const;
    Method getMethod() const;

    bool canHaveHttpBody() const;
    void appendBody(const char* in, const size_t len);

    bool isCorsPreflightRequest() const;
    CorsResponseHeaders getCorsResponseHeaders() const;

    Request& getRequest() { return request; }
    const Request& getRequest() const { return request; }

    void delayResponse();

    int write(const Response& response, const CorsResponseHeaders& cors);

    bool delayedResponseSet = false;
    std::future<Response> delayedResponse;

private:
    Channel channel;
    Request request;
    size_t contentLength = 0;
    CorsRequestHeaders corsHeaders;

    bool _canHaveHttpBody(Method m) const;
    bool _hasCorsPreflightHeaders() const;
};
}
}

#endif
