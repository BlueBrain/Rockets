/* Copyright (c) 2017, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
 *                     Stefan.Eilemann@epfl.ch
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

#ifndef ROCKETS_HTTP_TYPES_H
#define ROCKETS_HTTP_TYPES_H

#include <functional>
#include <future>

namespace rockets
{
namespace http
{
struct Request;
struct Response;
class Client;

/** HTTP method used in a Request. */
enum class Method
{
    GET,
    POST,
    PUT,
    PATCH,
    DELETE,
    OPTIONS,
    ALL //!< @internal, must be last
};

/** HTTP headers which can be used in a Response. */
enum class Header
{
    ALLOW,
    CONTENT_TYPE,
    LAST_MODIFIED,
    LOCATION,
    RETRY_AFTER
};

/** HTTP codes to be used in a Response. */
enum Code
{
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    PARTIAL_CONTENT = 206,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    MOVED_TEMPORARILY = 302,
    NOT_MODIFIED = 304,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    NOT_SUPPORTED = 405,
    NOT_ACCEPTABLE = 406,
    REQUEST_TIMEOUT = 408,
    PRECONDITION_FAILED = 412,
    UNSATISFIABLE_RANGE = 416,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    SPACE_UNAVAILABLE = 507
};

/** HTTP REST callback with Request parameter returning a Response future. */
using RESTFunc = std::function<std::future<Response>(const Request&)>;
}
}

#endif
