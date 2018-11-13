/* Copyright (c) 2017-2018, EPFL/Blue Brain Project
 *                          Raphael.Dumusc@epfl.ch
 *                          Daniel.Nachbaur@epfl.ch
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

#include "serverContext.h"

#include "http/connection.h"
#include "unavailablePortError.h"
#include "ws/connection.h"

#include <string.h> // memset

#if LWS_LIBRARY_VERSION_NUMBER >= 3000000
#define USE_EXPLICIT_VHOST 1
#endif

// was renamed in version 2.4
// https://github.com/warmcat/libwebsockets/commit/fc995df
#ifdef LWS_USE_LIBUV
#define LWS_WITH_LIBUV
#endif

namespace rockets
{
#ifdef LWS_WITH_LIBUV
#if LWS_LIBRARY_VERSION_NUMBER < 2000000
void signal_cb(uv_signal_t*, int)
{
}
#endif
#endif
ServerContext::ServerContext(const std::string& uri, const std::string& name,
                             const unsigned int threadCount,
                             lws_callback_function* callback,
                             lws_callback_function* wsCallback, void* user,
                             void* uvLoop)
    : protocols{make_protocol("http", callback, user), null_protocol()}
    , wsProtocolName{name}
{
    if (!wsProtocolName.empty() && wsCallback)
        createWebsocketsProtocol(wsCallback, user);

    fillContextInfo(uri, threadCount);

#ifdef LWS_WITH_LIBUV
    auto uvLoop_ = static_cast<uv_loop_t*>(uvLoop);
    const bool uvLoopRunning = uvLoop && uv_loop_alive(uvLoop_) != 0;
    if (uvLoop && !uvLoopRunning)
        throw std::runtime_error(
            "provided libuv loop either not valid or not running");

    if (uvLoop)
    {
#if LWS_LIBRARY_VERSION_NUMBER >= 3000000
        info.foreign_loops = &uvLoop;
#endif
        info.options |= LWS_SERVER_OPTION_LIBUV;
    }
#else
    if (uvLoop)
        throw std::runtime_error("libwebsockets has no support for libuv");
#endif

    context.reset(lws_create_context(&info));
    if (!context)
        throw std::runtime_error("libwebsocket init failed");

#if USE_EXPLICIT_VHOST
    // create vhost explicitly to retrieve port number which is no longer filled
    try
    {
        auto default_vhost = lws_create_vhost(context.get(), &info);
        info.port = lws_get_vhost_listen_port(default_vhost);
    }
    catch (const unavailable_port_error&)
    {
        std::rethrow_exception(std::current_exception());
    }
#endif

#ifdef LWS_WITH_LIBUV
#if LWS_LIBRARY_VERSION_NUMBER < 3000000
    if (uvLoopRunning)
    {
        lws_uv_sigint_cfg(context.get(), 0, nullptr);
#if LWS_LIBRARY_VERSION_NUMBER < 2000000
        lws_uv_initloop(context.get(), uvLoop_, &signal_cb, 0);
#else
        lws_uv_initloop(context.get(), uvLoop_, 0);
#endif
    }
#endif
#endif
}

std::string ServerContext::getHostname() const
{
    return interface.empty() ? rockets::getHostname() : getIP(interface);
}

uint16_t ServerContext::getPort() const
{
    return info.port;
}

int ServerContext::getThreadCount() const
{
    return lws_get_count_threads(context.get());
}

void ServerContext::requestBroadcast()
{
    lws_callback_on_writable_all_protocol(context.get(), &protocols[1]);
}

bool ServerContext::service(const int tsi, const int timeout_ms)
{
    return lws_service_tsi(context.get(), timeout_ms, tsi) >= 0;
}

void ServerContext::service(const int timeout_ms)
{
    lws_service(context.get(), timeout_ms);
}

void ServerContext::service(PollDescriptors& pollDescriptors,
                            const SocketDescriptor fd, const int events)
{
    pollDescriptors.service(context.get(), fd, events);
}

void ServerContext::cancelService()
{
    lws_cancel_service(context.get());
}

void ServerContext::createWebsocketsProtocol(lws_callback_function* wsCallback,
                                             void* user)
{
    protocols.insert(protocols.begin() + 1,
                     make_protocol(wsProtocolName.c_str(), wsCallback, user));
}

void ServerContext::fillContextInfo(const std::string& uri,
                                    const unsigned int threadCount)
{
    memset(&info, 0, sizeof(info));
    const auto parsedUri = parse(uri);
    interface = getInterface(parsedUri.host);
    if (!interface.empty())
        info.iface = interface.c_str();
    info.port = parsedUri.port;
    info.protocols = protocols.data();
    info.gid = -1;
    info.uid = -1;
#if USE_EXPLICIT_VHOST
    info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
#endif
    // header size: accommodate long "Authorization: Negotiate <kerberos token>"
    info.max_http_header_data = 8192;
    // service threads
    info.count_threads = threadCount;
#if LWS_LIBRARY_VERSION_NUMBER < 3000000
    // https://github.com/warmcat/libwebsockets/issues/1249
    info.max_http_header_pool = 1024;
#endif
}
}
