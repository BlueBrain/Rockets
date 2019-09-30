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

#ifndef ROCKETS_HTTP_REQUEST_H
#define ROCKETS_HTTP_REQUEST_H

#include <rockets/http/types.h>

#include <map>
#include <string>

namespace rockets
{
namespace http
{
/**
 * HTTP Request with method, path and body.
 *
 * The path provides the url part after the registered endpoint if it is
 * terminated with a slash.
 * Registered endpoint || HTTP request         || path
 * "api/windows/"      || "api/windows/jf321f" || "jf321f".
 * "api/windows/"      || "api/windows/"       || ""
 *
 * If an endpoint is not terminated with a slash, then only exactly matching
 * HTTP request will be processed.
 * Registered endpoint || HTTP request          || path
 * "api/objects"       || "api/objects"         || ""
 * "api/objects"       || "api/objects/abc"     || ** ENDPOINT NOT FOUND: 404 **
 *
 * The query is the url part after "?".
 * Registered endpoint || HTTP request                 || query    || path
 * "api/objects"       || "api/objects?size=4"         || "size=4" || ""
 * "api/windows/"      || "api/windows/jf321f?size=4"  || "size=4" || "jf321"
 *
 * The body is the HTTP request payload.
 */
struct Request
{
    Method method;
    std::string path;
    std::string origin;
    std::string host;
    std::map<std::string, std::string> query;
    std::string body;
};
} // namespace http
} // namespace rockets

#endif
