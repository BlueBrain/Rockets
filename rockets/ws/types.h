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

#ifndef ROCKETS_WS_TYPES_H
#define ROCKETS_WS_TYPES_H

#include <functional>
#include <string>
#include <vector>

namespace rockets
{
namespace ws
{
class Connection;

/**
 * The possible websocket message formats.
 */
enum class Format
{
    text,
    binary,
    unspecified
};

/**
 * The different types of recipients for a response.
 */
enum class Recipient
{
    sender,
    others,
    all
};

/**
 * A response followed after an incoming message/request.
 */
struct Response
{
    Response() = default;

    Response(const char* message_)
        : message(message_)
    {
    }

    Response(const std::string& message_,
             const Recipient recipient_ = Recipient::sender,
             const Format format_ = Format::unspecified)
        : message(message_)
        , recipient(recipient_)
        , format(format_)
    {
    }

    std::string message;
    Recipient recipient = Recipient::sender;
    Format format = Format::unspecified; // derive from request format
};

/** Callback for asynchronously responding to a message. */
using ResponseCallback = std::function<void(std::string)>;

/** WebSocket callback for handling text / binary messages. */
using MessageCallback = std::function<Response(std::string)>;

/** Callback for handling messge with delayed response. */
using MessageCallbackAsync = std::function<void(std::string, ResponseCallback)>;

/** Websocket callback for handling connection (open/close) messages. */
using ConnectionCallback = std::function<std::vector<Response>()>;
}
}

#endif
