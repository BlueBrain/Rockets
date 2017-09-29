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

#ifndef ROCKETS_SOCKETBASEDINTERFACE_H
#define ROCKETS_SOCKETBASEDINTERFACE_H

#include <rockets/api.h>
#include <rockets/types.h>

namespace rockets
{
/**
 * Base class for all socket-based interfaces (clients and server).
 */
class SocketBasedInterface
{
public:
    ROCKETS_API virtual ~SocketBasedInterface() = default;

    /**
     * Set a listener for integrating the server in an external poll array.
     *
     * @param listener to set, nullptr to remove.
     * @sa processSocket
     */
    ROCKETS_API void setSocketListener(SocketListener* listener)
    {
        _setSocketListener(listener);
    }

    /**
     * Process pending data on a given socket.
     *
     * @param fd socket with data to process.
     * @param events to process, must be POLLIN | POLLOUT according to return
     *        value of poll() operation for the fd.
     * @sa setSocketListener
     */
    ROCKETS_API void processSocket(const SocketDescriptor fd, const int events)
    {
        _processSocket(fd, events);
    }

    /**
     * Process all sockets using the internal poll array.
     *
     * @param timeout_ms maximum time allowed before returning.
     */
    ROCKETS_API void process(int timeout_ms) { _process(timeout_ms); }
private:
    virtual void _setSocketListener(SocketListener* listener) = 0;
    virtual void _processSocket(SocketDescriptor fd, int events) = 0;
    virtual void _process(int timeout_ms) = 0;
};
}

#endif
