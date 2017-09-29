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

#ifndef ROCKETS_SOCKETLISTENER_H
#define ROCKETS_SOCKETLISTENER_H

#include <rockets/api.h>
#include <rockets/types.h>

namespace rockets
{
/**
 * File descriptor listener for integration in external poll array.
 */
class SocketListener
{
public:
    ROCKETS_API virtual ~SocketListener() {}
    /**
     * Called when a new socket opens for a client connection.
     *
     * Call process(fd) when new data is available on the socket.
     *
     * @param fd socket descriptor for polling the connection.
     * @param events to poll() for, can be POLLIN | POLLOUT.
     * @sa onDeleteSocket
     */
    virtual void onNewSocket(SocketDescriptor fd, int events) = 0;

    /**
     * Called when the events to poll for have changed for a socket.
     *
     * @param fd socket descriptor for which to update polling events.
     * @param events to poll() for, can be POLLIN | POLLOUT.
     */
    virtual void onUpdateSocket(SocketDescriptor fd, int events) = 0;

    /**
     * Called when a client connection closes.
     *
     * @param fd socket descriptor to deregister.
     * @sa onNewSocket
     */
    virtual void onDeleteSocket(SocketDescriptor fd) = 0;
};
}
#endif
