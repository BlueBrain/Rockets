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

#ifndef ROCKETS_HTTP_REQUEST_HANDLER_H
#define ROCKETS_HTTP_REQUEST_HANDLER_H

#include <rockets/http/channel.h>
#include <rockets/http/types.h>

namespace rockets
{
namespace http
{
/**
 * HTTP request handler.
 */
class RequestHandler
{
public:
    RequestHandler(Channel&& channel, std::string body);

    std::future<Response> getFuture();

    int writeHeaders(unsigned char** buffer, const size_t size);
    int writeBody();

    void readResponseHeaders();
    bool hasResponseBody() const;
    void appendToResponseBody(const char* data, const size_t size);

    void finish();
    void abort(std::runtime_error&& e);

private:
    Channel channel;
    std::string body;
    std::promise<Response> promise;
    Response response;
    size_t responseLength = 0;
};
}
}

#endif
