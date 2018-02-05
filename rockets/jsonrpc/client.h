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

#include <rockets/jsonrpc/emitter.h>
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
class Client : public Emitter, public Receiver, public Requester
{
public:
    Client(Communicator& comm)
        : communicator{comm}
    {
        communicator.handleText([this](const jsonrpc::Request& request_) {
            if (!processResponse(request_.message))
                _processNotification(request_);
            return std::string();
        });
    }

private:
    /** Emitter::_sendNotification */
    void _sendNotification(std::string json) final { _send(std::move(json)); }
    /** Requester::_sendRequest */
    void _sendRequest(std::string json) final { _send(std::move(json)); }
    void _processNotification(const jsonrpc::Request& request_)
    {
        process(request_, [this](std::string response) {
            // Note: response should normally be empty, as we don't expect to
            // receive requests from the server (only notifications).
            if (!response.empty())
                _send(std::move(response));
        });
    }

    void _send(std::string&& message)
    {
        communicator.sendText(std::move(message));
    }

    Communicator& communicator;
};
}
}

#endif
