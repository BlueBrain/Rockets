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

#ifndef ROCKETS_UTILS_H
#define ROCKETS_UTILS_H

#include <libwebsockets.h>

#include <memory>
#include <string>

namespace rockets
{
struct Uri
{
    std::string protocol;
    std::string host;
    uint16_t port;
    std::string path;
};
Uri parse(const std::string& uri);

lws_protocols make_protocol(const char* name, lws_callback_function* callback,
                            void* user);
lws_protocols null_protocol();

std::string getIP(const std::string& iface);
std::string getInterface(const std::string& hostnameOrIP);

std::string getHostname();
}

#endif
