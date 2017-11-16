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

#ifndef ROCKETS_CLIENTCONTEXT_H
#define ROCKETS_CLIENTCONTEXT_H

#include <rockets/http/types.h>
#include <rockets/pollDescriptors.h>
#include <rockets/utils.h>
#include <rockets/ws/types.h>

#include <libwebsockets.h>

#include <string>
#include <vector>

namespace rockets
{
/**
 * Common context for http and websockets clients.
 */
class ClientContext
{
public:
    ClientContext(lws_callback_function* callback, void* user);

    ~ClientContext();

    lws* startHttpRequest(http::Method method, const std::string& uri);

    std::unique_ptr<ws::Connection> connect(const std::string& uri,
                                            const std::string& protocol);

    void service(int timeout_ms);
    void service(PollDescriptors& pollDescriptors, SocketDescriptor fd,
                 int events);

private:
    lws_context* context = nullptr;
    lws_context_creation_info info;
    std::vector<lws_protocols> protocols;
    std::string wsProtocolName;

    lws_client_connect_info makeConnectInfo(const Uri& uri) const;
};
}

#endif
