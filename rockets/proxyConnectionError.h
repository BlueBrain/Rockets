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

#ifndef ROCKETS_PROXYCONNECTIONERROR_H
#define ROCKETS_PROXYCONNECTIONERROR_H

#include <libwebsockets.h>

#include <stdexcept>

// WAR: older versions of libwebsockets did not call
// LWS_CALLBACK_CLIENT_CONNECTION_ERROR on proxy connection error.
// To avoid that the client waits endlessly for the connection future to be
// ready, a proxy_connection_error is thrown from the log handler.

#define PROXY_CONNECTION_ERROR_THROWS (LWS_LIBRARY_VERSION_NUMBER < 2004000)

namespace rockets
{
class proxy_connection_error : public std::runtime_error
{
    using runtime_error::runtime_error;
};
}

#endif
