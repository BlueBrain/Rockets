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

#ifndef ROCKETS_JSONRPC_CLIENT_H
#define ROCKETS_JSONRPC_CLIENT_H

#include <rockets/jsonrpc/receiver.h>
#include <rockets/jsonrpc/requester.h>

namespace rockets
{
namespace jsonrpc
{
/**
 * JSON-RPC client.
 *
 * The client can be used over any communication channel that provides the
 * following methods:
 *
 * - void sendText(std::string message);
 *   Used to send notifications and requests to the server.
 *
 * - void handleText(ws::MessageCallback callback);
 *   Used to register a callback for processing the responses from the server
 *   when they arrive (non-blocking requests) as well as receiving
 *   notifications.
 */
template <typename Communicator>
class Client : public Requester, public Receiver
{
public:
    Client(Communicator& comm)
        : communicator{comm}
    {
        communicator.handleText(
            [ this, thisStatus =
                        std::weak_ptr<bool>{status} ](const Request& request_) {
                // Prevent access to *this* by a callback after the Client has
                // been destroyed (in non-multithreaded contexts).
                if (!thisStatus.expired())
                {
                    if (!processResponse(request_.message))
                        _processNotification(request_);
                }
                return std::string();
            });
    }

private:
    /** Notifier::_send */
    void _send(std::string json) final
    {
        communicator.sendText(std::move(json));
    }

    void _processNotification(const Request& request_) { process(request_); }
    Communicator& communicator;
    std::shared_ptr<bool> status = std::make_shared<bool>(true);
};
}
}

#endif
