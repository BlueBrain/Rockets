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

#ifndef ROCKETS_SERVERCONTEXT_H
#define ROCKETS_SERVERCONTEXT_H

#include <rockets/http/types.h>
#include <rockets/pollDescriptors.h>
#include <rockets/utils.h>
#include <rockets/ws/types.h>

#include <rockets/websockets.h>


#include <string>
#include <vector>

namespace rockets
{
/**
 * Server context for http and websockets protocols.
 */
class ServerContext
{
public:
    ServerContext(const std::string& uri, const std::string& name,
                  const unsigned int threadCount,
                  lws_callback_function* callback,
                  lws_callback_function* wsCallback, void* user,
                  void* uvLoop = nullptr);

    ~ServerContext();

    std::string getHostname() const;
    uint16_t getPort() const;
    int getThreadCount() const;

    void requestBroadcast();

    bool service(int tsi, int timeout_ms);
    void service(int timeout_ms);
    void service(PollDescriptors& pollDescriptors, SocketDescriptor fd,
                 int events);
    void cancelService();

private:
    lws_context* context = nullptr;
    std::string interface;
    lws_context_creation_info info;
    std::vector<lws_protocols> protocols;
    std::string wsProtocolName;

    void fillContextInfo(const std::string& uri,
                         const unsigned int threadCount);
    void createWebsocketsProtocol(lws_callback_function* wsCallback,
                                  void* user);
};
}

#endif
