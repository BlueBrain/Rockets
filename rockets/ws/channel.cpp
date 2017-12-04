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

#include "channel.h"

namespace rockets
{
namespace ws
{
namespace
{
lws_write_protocol _getProtocol(const Format format)
{
    return format == Format::text ? LWS_WRITE_TEXT : LWS_WRITE_BINARY;
}
}

Channel::Channel(lws* wsi_)
    : wsi{wsi_}
{
}

Format Channel::getCurrentMessageFormat() const
{
    return lws_frame_is_binary(wsi) ? Format::binary : Format::text;
}

void Channel::requestWrite()
{
    lws_callback_on_writable(wsi);
}

bool Channel::canWrite() const
{
    return !lws_send_pipe_choked(wsi);
}

bool Channel::currentMessageHasMore() const
{
    return !lws_is_final_fragment(wsi);
}

size_t Channel::getCurrentMessageRemainingSize() const
{
    return lws_remaining_packet_payload(wsi);
}

void Channel::write(std::string&& message, const Format format)
{
    const auto size = message.size();
    message.insert(0, LWS_PRE, '0');
    auto data = (unsigned char*)(&message.data()[LWS_PRE]);
    const auto protocol = _getProtocol(format);
    lws_write(wsi, data, size, protocol);
}
}
}
