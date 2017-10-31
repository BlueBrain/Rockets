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

#include "pollDescriptors.h"

#include "socketListener.h"

namespace rockets
{
void PollDescriptors::add(const lws_pollargs* pa)
{
    const auto fd = pa->fd;
    if (pollDescriptors.count(fd))
        return;

    pollDescriptors[fd].fd = fd;
    pollDescriptors[fd].events = pa->events;
    pollDescriptors[fd].revents = pa->prev_events;
    if (_listener)
        _listener->onNewSocket(fd, pa->events);
}

void PollDescriptors::update(const lws_pollargs* pa)
{
    const auto fd = pa->fd;
    if (!pollDescriptors.count(fd))
        return;

    pollDescriptors[fd].events = pa->events;
    pollDescriptors[fd].revents = pa->prev_events;
    if (_listener)
        _listener->onUpdateSocket(fd, pa->events);
}

void PollDescriptors::remove(const lws_pollargs* pa)
{
    const auto fd = pa->fd;
    if (!pollDescriptors.count(fd))
        return;

    if (_listener)
        _listener->onDeleteSocket(fd);
    pollDescriptors.erase(fd);
}

void PollDescriptors::setListener(SocketListener* listener)
{
    _listener = listener;
    if (_listener)
    {
        for (const auto& fd : pollDescriptors)
            _listener->onNewSocket(fd.first, fd.second.events);
    }
}

void PollDescriptors::service(lws_context* context, const int fd,
                              const int events)
{
    const auto socket = pollDescriptors.find(fd);
    if (socket == pollDescriptors.end())
        return;

    socket->second.revents = events;
    lws_service_fd(context, &socket->second);

#if LWS_LIBRARY_VERSION_NUMBER >= 2000000
    // if needed, force-service wsis that may not have read all input
    while (!lws_service_adjust_timeout(context, 1, 0))
        lws_service_tsi(context, -1, 0);
#endif
}
}
