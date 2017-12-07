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

#include "requestHandler.h"

namespace rockets
{
namespace http
{
RequestHandler::RequestHandler(Channel&& channel_, std::string body_)
    : channel{std::move(channel_)}
    , body{std::move(body_)}
{
}

std::future<Response> RequestHandler::getFuture()
{
    return promise.get_future();
}

int RequestHandler::writeHeaders(unsigned char** buffer, const size_t size)
{
    return channel.writeRequestHeader(body, buffer, size);
}

#if LWS_LIBRARY_VERSION_NUMBER >= 2001000
int RequestHandler::writeBody()
{
    return channel.writeRequestBody(body);
}
#endif

void RequestHandler::readResponseHeaders()
{
#if LWS_LIBRARY_VERSION_NUMBER >= 2001000
    response.code = channel.readResponseCode();
#endif
    response.headers = channel.readResponseHeaders();
    responseLength = channel.readContentLength();
}

void RequestHandler::appendToResponseBody(const char* data, const size_t size)
{
    response.body.append(data, size);
}

void RequestHandler::finish()
{
    promise.set_value(std::move(response));
}

void RequestHandler::abort(std::runtime_error&& e)
{
    promise.set_exception(std::make_exception_ptr(e));
}

bool RequestHandler::hasResponseBody() const
{
    return responseLength > 0;
}
}
}
