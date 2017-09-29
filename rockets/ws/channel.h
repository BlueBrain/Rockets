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

#ifndef ROCKETS_WS_CHANNEL_H
#define ROCKETS_WS_CHANNEL_H

#include <rockets/ws/types.h>

#include <libwebsockets.h>

namespace rockets
{
namespace ws
{
/**
 * A WebSocket communication channel.
 *
 * Wraps libwebsockets read/write methods for use on both Client and Server.
 */
class Channel
{
public:
    explicit Channel(lws* wsi);

    Format getCurrentMessageFormat() const;

    void requestWrite();
    void write(std::string&& message, Format format);

private:
    lws* wsi = nullptr;
};
}
}

#endif
