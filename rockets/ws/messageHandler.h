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

#ifndef ROCKETS_WS_MESSAGEHANDLER_H
#define ROCKETS_WS_MESSAGEHANDLER_H

#include <rockets/ws/types.h>

#include <libwebsockets.h>
#include <map>

namespace rockets
{
namespace ws
{
using Connections = std::map<lws*, ConnectionPtr>;

/**
 * Handle message callbacks for text/binary messages.
 */
class MessageHandler
{
public:
    MessageHandler() = default;
    MessageHandler(const Connections& connections);

    /**
     * Handle a new connection.
     *
     * @param connection the connection to use for reply.
     */
    void handleOpenConnection(ConnectionPtr connection);

    /**
     * Handle close of a connection.
     *
     * @param connection the connection to use for reply.
     */
    void handleCloseConnection(ConnectionPtr connection);

    /**
     * Handle an incomming message for the given connection.
     *
     * @param connection the connection to use for reply.
     * @param data the incoming data pointer.
     * @param len the length of the data.
     */
    void handleMessage(ConnectionPtr connection, const char* data, size_t len);

    /** The callback for incoming connections. */
    ConnectionCallback callbackOpen;

    /** The callback for closing connections. */
    ConnectionCallback callbackClose;

    /** The callback for messages in text format. */
    MessageCallback callbackText;

    /** The callback for messages in text format with async response. */
    MessageCallbackAsync callbackTextAsync;

    /** The callback for messages in binary format. */
    MessageCallback callbackBinary;

private:
    void _sendResponseToRecipient(const Response& response,
                                  ConnectionPtr connection);

    static Connections _emptyConnections;
    const Connections& _connections{_emptyConnections};
    std::string _buffer;
};
}
}

#endif
