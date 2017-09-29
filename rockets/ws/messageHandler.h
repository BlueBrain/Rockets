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

#ifndef ROCKETS_WS_MESSAGEHANDLER_H
#define ROCKETS_WS_MESSAGEHANDLER_H

#include <rockets/ws/types.h>

namespace rockets
{
namespace ws
{
/**
 * Handle message callbacks for text/binary messages.
 */
class MessageHandler
{
public:
    /**
     * Handle an incomming message for the given connection.
     *
     * @param connection the connection to use for reply.
     * @param data the incoming data pointer.
     * @param len the length of the data.
     * @param format the format of the data.
     */
    void handle(Connection& connection, const char* data, size_t len,
                Format format);

    /** The callback for messages in text format. */
    MessageCallback callbackText;

    /** The callback for messages in binary format. */
    MessageCallback callbackBinary;
};
}
}

#endif
