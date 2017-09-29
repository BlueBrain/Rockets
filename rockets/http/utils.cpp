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

#include "utils.h"

#include <libwebsockets.h>

namespace rockets
{
namespace http
{
std::string to_string(const CorsResponseHeader header)
{
    switch (header)
    {
    case CorsResponseHeader::access_control_allow_headers:
        return "Access-Control-Allow-Headers";
    case CorsResponseHeader::access_control_allow_methods:
        return "Access-Control-Allow-Methods";
    case CorsResponseHeader::access_control_allow_origin:
        return "Access-Control-Allow-Origin";
    default:
        throw std::logic_error("no such header");
    }
}

const char* to_cstring(const Method method)
{
    switch (method)
    {
    case Method::GET:
        return "GET";
    case Method::POST:
        return "POST";
    case Method::PUT:
        return "PUT";
    case Method::PATCH:
        return "PATCH";
    case Method::DELETE:
        return "DELETE";
    case Method::OPTIONS:
        return "OPTIONS";
    default:
        throw std::logic_error("unsupported http method");
    }
}
}
}
