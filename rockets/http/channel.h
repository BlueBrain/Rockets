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

#ifndef ROCKETS_HTTP_CHANNEL_H
#define ROCKETS_HTTP_CHANNEL_H

#include <rockets/http/cors.h>
#include <rockets/http/request.h>
#include <rockets/http/response.h>
#include <rockets/http/types.h>

#include <libwebsockets.h>

namespace rockets
{
namespace http
{
/**
 * An HTTP communication channel.
 *
 * Provides low-level HTTP read/write methods for the Client and Server over a
 * given libwebsockets connection.
 */
class Channel
{
public:
    explicit Channel(lws* wsi);
    Channel() = default;

    /* Client + Server */
    size_t readContentLength() const;

    /* Server */
    Method readMethod() const;
    std::string readOrigin() const;
    std::map<std::string, std::string> readQueryParameters() const;
    CorsRequestHeaders readCorsRequestHeaders() const;
    void requestCallback();
    int writeResponseHeaders(const CorsResponseHeaders& corsHeaders,
                             const Response& response);
    int writeResponseBody(const Response& response);

    /* Client */
    int writeRequestHeader(const std::string& body, unsigned char** buffer,
                           size_t bufferSize);
#if LWS_LIBRARY_VERSION_NUMBER >= 2001000
    int writeRequestBody(const std::string& body);
    Code readResponseCode() const;
#endif
    Response::Headers readResponseHeaders() const;

private:
    lws* wsi = nullptr;

    std::string _readHeader(lws_token_indexes token) const;
    void _writeResponseBody(const std::string& message);
    bool _write(const std::string& message, lws_write_protocol protocol);
};
}
}

#endif
