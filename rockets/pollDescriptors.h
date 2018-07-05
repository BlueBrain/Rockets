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

#ifndef ROCKETS_POLLDESCRIPTORS_H
#define ROCKETS_POLLDESCRIPTORS_H

#include "types.h"

#include <rockets/websockets.h>

#include <map>

namespace rockets
{
/**
 * Poll descriptors for integration of server and client in external poll array.
 */
class PollDescriptors
{
public:
    void add(const lws_pollargs* pa);
    void update(const lws_pollargs* pa);
    void remove(const lws_pollargs* pa);

    void setListener(SocketListener* listener);

    void service(lws_context* context, lws_sockfd_type fd, int events);

private:
    std::map<lws_sockfd_type, lws_pollfd> pollDescriptors;
    SocketListener* _listener = nullptr;
};
}

#endif
