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

#ifndef ROCKETS_HTTP_CLIENT_H
#define ROCKETS_HTTP_CLIENT_H

#include <rockets/socketBasedInterface.h>
#include <rockets/http/request.h>
#include <rockets/http/response.h>

namespace rockets
{
namespace http
{
/**
 * Client for making asynchronous HTTP requests.
 *
 * Example usage: @include apps/httpRequest.cpp
 */
class Client : public SocketBasedInterface
{
public:
    /** @name Setup */
    //@{
    /** Construct a new client. */
    ROCKETS_API Client();

    /** Close the client. */
    ROCKETS_API ~Client();
    //@}

    /**
     * Make an http request.
     *
     * @param uri to address the request.
     * @param method http method to use.
     * @param body optional payload to send.
     * @throw std::invalid_argument if the uri is too long (>4000 char) or
     *        some parameter is invalid or not supported.
     * @return future http response - can be a std::runtime_error if the
     *         request fails.
     */
    ROCKETS_API std::future<http::Response> request(
        const std::string& uri, http::Method method = http::Method::GET,
        std::string body = std::string());

    class Impl; // must be public for static_cast from C callback
private:
    std::unique_ptr<Impl> _impl;

    void _setSocketListener(SocketListener* listener) final;
    void _processSocket(SocketDescriptor fd, int events) final;
    void _process(int timeout_ms) final;
};
}
}

#endif
