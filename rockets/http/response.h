/* Copyright (c) 2017, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
 *                     Pawel.Podhajski@epfl.ch
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

#ifndef ROCKETS_HTTP_RESPONSE_H
#define ROCKETS_HTTP_RESPONSE_H

#include <rockets/http/types.h>

#include <map>    // member
#include <string> // member

namespace rockets
{
namespace http
{
/**
 * Response to an HTTP Request.
 */
struct Response
{
    /** HTTP return code. */
    Code code;

    /** Payload to return in a format specified in CONTENT_TYPE header. */
    std::string body;

    /** HTTP message headers. */
    using Headers = std::map<Header, std::string>;
    Headers headers;

    /** Construct a Response with a given return code and payload. */
    Response(const Code code_ = Code::OK, std::string body_ = std::string())
        : code{code_}
        , body{std::move(body_)}
    {
    }

    /** Construct a Response with a given code, payload and content type. */
    Response(const Code code_, std::string body_,
             const std::string& contentType)
        : code{code_}
        , body{std::move(body_)}
        , headers{{Header::CONTENT_TYPE, contentType}}
    {
    }

    /** Construct a Response with a given code, payload and map of headers. */
    Response(const Code code_, std::string body_,
             std::map<Header, std::string> headers_)
        : code{code_}
        , body{std::move(body_)}
        , headers{{std::move(headers_)}}
    {
    }
};
}
}

#endif
