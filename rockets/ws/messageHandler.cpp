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

#include "messageHandler.h"

#include "channel.h"
#include "connection.h"

#include <algorithm>

namespace rockets
{
namespace ws
{
Connections MessageHandler::_emptyConnections{};

MessageHandler::MessageHandler(const Connections& connections)
    : _connections(connections)
{
}

void MessageHandler::handleMessage(ConnectionPtr connection, const char* data,
                                   const size_t len)
{
    // compose potentially fragmented message
    if (connection->getChannel().currentMessageHasMore() && _buffer.empty())
    {
        _buffer.reserve(
            len + connection->getChannel().getCurrentMessageRemainingSize());
    }
    _buffer.append(data, len);
    if (connection->getChannel().currentMessageHasMore())
        return;

    const auto clientID = reinterpret_cast<uintptr_t>(connection.get());
    const Format format = connection->getChannel().getCurrentMessageFormat();
    Response response;
    if (format == Format::text)
    {
        if (callbackText)
            response = callbackText({std::move(_buffer), clientID});
        else if (callbackTextAsync)
        {
            callbackTextAsync({std::move(_buffer), clientID},
                              [weak = std::weak_ptr<Connection>(connection)](
                                  const auto& reply) {
                                  auto conn = weak.lock();
                                  if (conn && !reply.empty())
                                      conn->sendText(std::move(reply));
                              });
            return;
        }
    }
    else if (format == Format::binary && callbackBinary)
        response = callbackBinary({std::move(_buffer), clientID});

    _buffer.clear();

    if (response.format == Format::unspecified)
        response.format = format;
    _sendResponseToRecipient(response, connection);
}

void MessageHandler::handleOpenConnection(ConnectionPtr connection)
{
    if (!callbackOpen)
        return;

    const auto clientID = reinterpret_cast<uintptr_t>(connection.get());
    auto responses = callbackOpen(clientID);
    for (auto& response : responses)
        _sendResponseToRecipient(response, connection);
}

void MessageHandler::handleCloseConnection(ConnectionPtr connection)
{
    if (!callbackClose)
        return;

    const auto clientID = reinterpret_cast<uintptr_t>(connection.get());
    auto responses = callbackClose(clientID);
    for (auto& response : responses)
        _sendResponseToRecipient(response, connection);
}

void sendResponse(const Response& response, Connection& connection)
{
    switch (response.format)
    {
    case Format::unspecified:
        break;
    case Format::binary:
        connection.sendBinary(std::move(response.message));
        break;
    case Format::text:
    default:
        connection.sendText(std::move(response.message));
    }
}

void MessageHandler::_sendResponseToRecipient(const Response& response,
                                              ConnectionPtr sender)
{
    if (response.message.empty())
        return;

    switch (response.recipient)
    {
    case Recipient::all:
    case Recipient::others:
    {
        for (const auto& connection : _connections)
        {
            if (response.recipient == Recipient::others &&
                connection.second == sender)
            {
                continue;
            }
            sendResponse(response, *connection.second);
        }
        break;
    }
    case Recipient::sender:
    default:
        sendResponse(response, *sender);
    }
}
}
}
