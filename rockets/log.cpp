/* Copyright (c) 2017-2018, EPFL/Blue Brain Project
 *                          Raphael.Dumusc@epfl.ch
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

#include "proxyConnectionError.h"
#include "unavailablePortError.h"

#include <rockets/websockets.h>


#include <iostream>
#include <stdexcept>

namespace rockets
{
namespace
{
const std::string proxyError = "ERROR proxy: HTTP/1.1 503 \n";
#if LWS_LIBRARY_VERSION_NUMBER >= 3000000
const char portError[] = "ERROR on binding fd ";
#endif

void handleErrorMessage(int, const char* message)
{
#if PROXY_CONNECTION_ERROR_THROWS
    if (message == proxyError)
        throw proxy_connection_error(message);
#endif

#if LWS_LIBRARY_VERSION_NUMBER >= 3000000
    // Occurs during lws_create_vhost() if the chosen port is unavailable.
    // The returned vhost is valid(!) so the error can only be caught here.
    if (strncmp(message, portError, sizeof(portError) - 1) == 0)
        throw unavailable_port_error(message);
#endif

#ifdef NDEBUG
    (void)message;
#else
    std::cerr << "rockets: lws_err: " << message; // ends with \n
#endif
}
struct LogInitializer
{
    LogInitializer() { lws_set_log_level(LLL_ERR, handleErrorMessage); }
};
static LogInitializer silencer;
}
}
