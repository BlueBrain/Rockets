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

#ifndef ROCKETS_WS_CLIENT_H
#define ROCKETS_WS_CLIENT_H

#include <rockets/socketBasedInterface.h>
#include <rockets/ws/types.h>

namespace rockets
{
namespace ws
{
/**
 * Websocket client.
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

    /**
     * Connect to a websockets server.
     *
     * @param uri "hostname:port" to connect to.
     * @param protocol to use.
     * @return future that becomes ready when the connection is established -
     *         can be a std::runtime_error if the connection fails.
     */
    ROCKETS_API std::future<void> connect(const std::string& uri,
                                           const std::string& protocol);
    //@}

    /** Send a text message to the websocket server. */
    ROCKETS_API void sendText(std::string message);

    /** Send a binary message to the websocket server. */
    ROCKETS_API void sendBinary(const char* data, size_t size);

    /** Set a callback for handling text messages from the server. */
    ROCKETS_API void handleText(MessageCallback callback);

    /** Set a callback for handling binray messages from the server. */
    ROCKETS_API void handleBinary(MessageCallback callback);

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
