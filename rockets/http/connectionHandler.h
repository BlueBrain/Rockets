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

#ifndef ROCKETS_HTTP_CONNECTION_HANDLER_H
#define ROCKETS_HTTP_CONNECTION_HANDLER_H

#include <rockets/http/connection.h>
#include <rockets/http/filter.h>
#include <rockets/http/registry.h>
#include <rockets/http/types.h>

namespace rockets
{
namespace http
{
/**
 * Handle HTTP connection requests from clients.
 *
 * This class processes incoming HTTP payload until requests are complete,
 * then responds to them by calling an appropriate handler from the Registry,
 * or an error code otherwise.
 *
 * It also answers CORS preflight requests directly.
 *
 * Incoming connections can optionally be filtered out by setting a Filter.
 */
class ConnectionHandler
{
public:
    ConnectionHandler(const Registry& registry);
    void setFilter(const Filter* filter);

    int handleNewRequest(Connection& connection) const;
    int handleData(Connection& connection, const char* data, size_t size) const;
    int respondToRequest(Connection& connection) const;

private:
    const http::Filter* _filter = nullptr;
    const Registry& _registry;

    int _replyToCorsPreflightRequest(Connection& connection) const;
    std::future<Response> _generateResponse(Connection& connection) const;
    std::future<Response> _callHandler(const Connection& connection,
                                       const std::string& endpoint) const;
    CorsResponseHeaders _makeCorsPreflighResponseHeaders(
        const std::string& path) const;
};
}
}

#endif
