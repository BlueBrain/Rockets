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

#ifndef ROCKETS_WS_CONNECTION_H
#define ROCKETS_WS_CONNECTION_H

#include <rockets/ws/types.h>

#include <deque>
#include <memory>

namespace rockets
{
namespace ws
{
class Channel;

/**
 * A WebSocket connection.
 */
class Connection
{
public:
    explicit Connection(std::unique_ptr<Channel> channel);

    /** Send a text message (will be queued for later processing). */
    void sendText(std::string message);

    /** Send a binary message (will be queued for later processing). */
    void sendBinary(std::string message);

    /** Write all pending messages. */
    void writeMessages();

    /** Enqueue a text message. */
    void enqueueText(std::string message);

    /** Enqueue a binary message. */
    void enqueueBinary(std::string message);

private:
    std::unique_ptr<Channel> channel;
    std::deque<std::pair<std::string, Format>> out;

    bool hasMessage() const;
    void writeOneMessage();
};
}
}

#endif
