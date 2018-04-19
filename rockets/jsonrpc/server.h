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

#ifndef ROCKETS_JSONRPC_SERVER_H
#define ROCKETS_JSONRPC_SERVER_H

#include <rockets/jsonrpc/cancellableReceiver.h>
#include <rockets/jsonrpc/notifier.h>
#include <rockets/ws/types.h>

namespace rockets
{
namespace jsonrpc
{
/**
 * JSON-RPC server.
 *
 * The server can be used over any communication channel that provides the
 * following methods:
 *
 * - void broadcastText(std::string message);
 *   Used for sending notifications to connected clients.
 *
 * - void handleText(ws::MessageCallback callback);
 *   Used to register a callback for processing the requests and notifications
 *   coming from the client(s).
 */
template <typename CommunicatorT>
class Server : public Notifier, public CancellableReceiver
{
public:
    Server(CommunicatorT& server)
        : CancellableReceiver([&server](std::string json, uintptr_t client) {
            server.sendText(std::move(json), client);
        })
        , communicator{server}
    {
        communicator.handleText(
            [this](ws::Request request, ws::ResponseCallback callback) {
                process(std::move(request), callback);
            });
    }

private:
    /** Notifier::_send */
    void _send(std::string json) final
    {
        communicator.broadcastText(std::move(json));
    }

    CommunicatorT& communicator;
};
}
}

#endif
