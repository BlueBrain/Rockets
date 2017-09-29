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

#ifndef ROCKETS_HTTP_CORS_H
#define ROCKETS_HTTP_CORS_H

#include <rockets/http/types.h>

#include <map>
#include <string>

namespace rockets
{
namespace http
{
struct CorsRequestHeaders
{
    std::string origin;
    std::string accessControlRequestHeaders;
    Method accessControlRequestMethod = Method::ALL;
};

enum class CorsResponseHeader
{
    access_control_allow_headers,
    access_control_allow_methods,
    access_control_allow_origin
};
using CorsResponseHeaders = std::map<CorsResponseHeader, std::string>;
}
}

#endif
