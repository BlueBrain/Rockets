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
    _sendResponse(response, connection);
}

void MessageHandler::handleOpenConnection(ConnectionPtr connection)
{
    _connections.push_back(connection);
    if (!callbackOpen)
        return;

    const auto clientID = reinterpret_cast<uintptr_t>(connection.get());
    auto responses = callbackOpen(clientID);
    for (auto& response : responses)
        _sendResponse(response, connection);
}

void MessageHandler::handleCloseConnection(ConnectionPtr connection)
{
    auto end = std::remove_if(_connections.begin(), _connections.end(),
                              [&connection](auto conn) {
                                  return conn.lock() == connection;
                              });
    _connections.erase(end, _connections.end());

    if (!callbackClose)
        return;

    const auto clientID = reinterpret_cast<uintptr_t>(connection.get());
    auto responses = callbackClose(clientID);
    for (auto& response : responses)
        _sendResponse(response, connection);
}

void MessageHandler::_sendResponse(const Response& response,
                                   ConnectionPtr sender)
{
    if (response.message.empty())
        return;

    std::vector<std::weak_ptr<Connection>> connections;
    switch (response.recipient)
    {
    case Recipient::all:
        connections = _connections;
        break;
    case Recipient::others:
    {
        connections = _connections;
        auto end = std::remove_if(connections.begin(), connections.end(),
                                  [&sender](auto conn) {
                                      return conn.lock() == sender;
                                  });
        connections.erase(end, connections.end());
        break;
    }
    case Recipient::sender:
    default:
        connections.push_back(sender);
    }

    for (auto connection : connections)
    {
        switch (response.format)
        {
        case Format::unspecified:
            break;
        case Format::binary:
            if (auto conn = connection.lock())
                conn->sendBinary(std::move(response.message));
            break;
        case Format::text:
        default:
            if (auto conn = connection.lock())
                conn->sendText(std::move(response.message));
        }
    }
}
}
}
