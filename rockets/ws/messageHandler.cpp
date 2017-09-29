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

#include "messageHandler.h"

#include "connection.h"

namespace rockets
{
namespace ws
{
void MessageHandler::handle(Connection& connection, const char* data,
                            const size_t len, const Format format)
{
    if (format == Format::text && callbackText)
    {
        auto response = callbackText({data, len});
        if (!response.empty())
            connection.sendText(std::move(response));
    }
    else if (format == Format::binary && callbackBinary)
    {
        auto response = callbackBinary({data, len});
        if (!response.empty())
            connection.sendBinary(std::move(response));
    }
}
}
}
