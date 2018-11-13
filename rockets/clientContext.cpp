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

#include "clientContext.h"

#include "http/utils.h"
#include "ws/channel.h"
#include "ws/connection.h"

#include <sstream>
#include <string.h> // memset

#if LWS_LIBRARY_VERSION_NUMBER >= 2003000
#define CLIENT_CAN_DESTROY_VHOST 1
#endif

namespace
{
const char* contextInitFailure = "failed to initialize lws context";
#if CLIENT_USE_EXPLICIT_VHOST
const char* vhostInitFailure = "failed to initialize lws vhost";
#endif
const char* wsConnectionFailure = "server unreachable";
const size_t maxQuerySize = 4096 - 196 /*padding determined empirically*/;
const char* uriTooLong = "uri too long (max ~4000 char)";
#if LWS_LIBRARY_VERSION_NUMBER < 2000000
const char* onlyGetSupported = "Only GET is supported with lws < 2.0";
#endif

bool hasEnding(const std::string& string, const std::string& ending)
{
    if (string.length() < ending.length())
        return false;
    return string.compare(string.length() - ending.length(), ending.length(),
                          ending) == 0;
}

bool isNoProxyHost(const std::string& hostname)
{
    auto no_proxy = getenv("no_proxy");
    if (!no_proxy)
        return false;

    std::stringstream stream(no_proxy);
    std::string host;

    // no_proxy list is comma-separated, and can have wildcards
    while (std::getline(stream, host, ','))
    {
        if (host.empty())
            continue;
        if (host == hostname)
            return true;
        if (host[0] == '*')
        {
            host = host.substr(1);
            if (hasEnding(hostname, host))
                return true;
        }
    }
    return false;
}
}

namespace rockets
{
ClientContext::ClientContext(lws_callback_function* callback, void* user)
    : protocols{make_protocol(wsProtocolName.c_str(), callback, user),
                null_protocol()}
{
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols.data();
    info.gid = -1;
    info.uid = -1;
    info.max_http_header_data = 4096;
#if CLIENT_USE_EXPLICIT_VHOST
    info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
#endif
    createContext();
    createVhost();
}

void ClientContext::createContext()
{
    context.reset(lws_create_context(&info));
    if (!context)
        throw std::runtime_error(contextInitFailure);
}

void ClientContext::createVhost()
{
#if CLIENT_USE_EXPLICIT_VHOST
    if (vhost)
    {
#if CLIENT_CAN_DESTROY_VHOST
        lws_vhost_destroy(vhost);
#else // must recreate entire context
        createContext();
#endif
        vhost = nullptr;
    }

    vhost = lws_create_vhost(context.get(), &info);
    if (!vhost)
        throw std::runtime_error(vhostInitFailure);
#endif
}

lws* ClientContext::startHttpRequest(const http::Method method,
                                     const std::string& uri)
{
    if (uri.size() > maxQuerySize)
        throw std::invalid_argument(uriTooLong);

    const auto parsedUri = parse(uri);
    auto connectInfo = makeConnectInfo(parsedUri);

#if LWS_LIBRARY_VERSION_NUMBER < 2000000
    if (method != http::Method::GET)
        throw std::invalid_argument(onlyGetSupported);
#else
    connectInfo.method = http::to_cstring(method);
#endif

    if (isNoProxyHost(parsedUri.host))
        disableProxy();

    return lws_client_connect_via_info(&connectInfo);
}

std::unique_ptr<ws::Connection> ClientContext::connect(
    const std::string& uri, const std::string& protocol)
{
#if LWS_LIBRARY_VERSION_NUMBER < 2004000
    const bool recreateVhost = wsProtocolName != protocol;
#endif
    wsProtocolName = protocol;

#if LWS_LIBRARY_VERSION_NUMBER < 2004000
    protocols[0].name = wsProtocolName.c_str();
    if (recreateVhost)
        createVhost();
#endif

    const auto parsedUri = parse(uri);
    auto connectInfo = makeConnectInfo(parsedUri);
    connectInfo.protocol = wsProtocolName.c_str();

    if (isNoProxyHost(parsedUri.host))
        disableProxy();

    if (auto wsi = lws_client_connect_via_info(&connectInfo))
        return std::make_unique<ws::Connection>(
            std::make_unique<ws::Channel>(wsi));

    throw std::runtime_error(wsConnectionFailure);
}

void ClientContext::service(const int timeout_ms)
{
    lws_service(context.get(), timeout_ms);
}

void ClientContext::service(PollDescriptors& pollDescriptors,
                            const SocketDescriptor fd, const int events)
{
    pollDescriptors.service(context.get(), fd, events);
}

lws_client_connect_info ClientContext::makeConnectInfo(const Uri& uri) const
{
    lws_client_connect_info c_info;
    memset(&c_info, 0, sizeof(c_info));

    c_info.context = context.get();
    c_info.ietf_version_or_minus_one = -1;

    c_info.address = uri.host.c_str();
    c_info.port = uri.port ? uri.port : 80;
    c_info.path = uri.path.c_str();

    c_info.host = c_info.address;
    c_info.origin = lws_canonical_hostname(context.get());

    return c_info;
}

void ClientContext::disableProxy()
{
#if CLIENT_USE_EXPLICIT_VHOST
    lws_set_proxy(vhost, ":0");
#else
    lws_set_proxy(context.get(), ":0");
#endif
}
}
