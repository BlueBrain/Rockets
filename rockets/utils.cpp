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

#include "utils.h"

#include <lws_config.h>

namespace rockets
{
Uri parse(std::string uri)
{
    auto in = const_cast<char*>(uri.c_str()); // uri is modified by parsing
    const char* protocol = nullptr;
    const char* address = nullptr;
    int port = 0;
    const char* path = nullptr;
    if (lws_parse_uri(in, &protocol, &address, &port, &path))
        throw std::invalid_argument("invalid uri");
    if (port < 0 || port > UINT16_MAX)
        throw std::invalid_argument("uri has invalid port range");
    return {protocol, address, (uint16_t)port, std::string("/").append(path)};
}

lws_protocols make_protocol(const char* name, lws_callback_function* callback,
                            void* user)
{
    // clang-format off
    return lws_protocols{ name, callback, 0, 0, 0, user
        #if LWS_LIBRARY_VERSION_NUMBER >= 2003000
                , 0
        #endif
    };
    // clang-format on
}

lws_protocols null_protocol()
{
    return make_protocol(nullptr, nullptr, nullptr);
}
}
