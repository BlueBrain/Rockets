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

#include "connection.h"

#include "channel.h"

namespace rockets
{
namespace ws
{
Connection::Connection(std::unique_ptr<Channel> channel_)
    : channel{std::move(channel_)}
{
}

void Connection::sendText(std::string message)
{
    enqueueText(std::move(message));
    channel->requestWrite();
}

void Connection::sendBinary(std::string message)
{
    enqueueBinary(std::move(message));
    channel->requestWrite();
}

void Connection::writeMessages()
{
    while (hasMessage() && channel->canWrite())
        writeOneMessage();

    if (hasMessage())
        channel->requestWrite();
}

void Connection::enqueueText(std::string message)
{
    out.emplace_back(std::move(message), Format::text);
}

void Connection::enqueueBinary(std::string message)
{
    out.emplace_back(std::move(message), Format::binary);
}

bool Connection::hasMessage() const
{
    return !out.empty();
}

void Connection::writeOneMessage()
{
    auto& message = out.at(0);
    channel->write(std::move(message.first), message.second);
    out.pop_front();
}
}
}
