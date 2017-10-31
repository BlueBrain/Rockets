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

#include "clientContext.h"

#include "http/utils.h"
#include "ws/channel.h"
#include "ws/connection.h"

namespace
{
const char* contextInitFailure = "failed to initialize lws context";
const char* wsConnectionFailure = "server unreachable";
const size_t maxQuerySize = 4096 - 96 /*padding determined empirically*/;
const char* uriTooLong = "uri too long (max ~4000 char)";
#if LWS_LIBRARY_VERSION_NUMBER < 2000000
const char* onlyGetSupported = "Only GET is supported with lws < 2.0";
#endif
}

namespace rockets
{

ClientContext::ClientContext(lws_callback_function* callback, void* user)
    : protocols{make_protocol("default", callback, user), null_protocol()}
{
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols.data();
    info.gid = -1;
    info.uid = -1;
    context = lws_create_context(&info);
    if (!context)
        throw std::runtime_error(contextInitFailure);
}

ClientContext::~ClientContext()
{
    lws_context_destroy(context);
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

    return lws_client_connect_via_info(&connectInfo);
}

std::unique_ptr<ws::Connection>
ClientContext::connect(const std::string& uri, const std::string& protocol)
{
    wsProtocolName = protocol;
    protocols[0].name = wsProtocolName.c_str();

    const auto parsedUri = parse(uri);
    auto connectInfo = makeConnectInfo(parsedUri);
    connectInfo.protocol = wsProtocolName.c_str();

    if (auto wsi = lws_client_connect_via_info(&connectInfo))
        return make_unique<ws::Connection>(make_unique<ws::Channel>(wsi));

    throw std::runtime_error(wsConnectionFailure);
}

void ClientContext::service(const int timeout_ms)
{
    lws_service(context, timeout_ms);
}

void ClientContext::service(PollDescriptors& pollDescriptors,
                            const SocketDescriptor fd, const int events)
{
    pollDescriptors.service(context, fd, events);
}

lws_client_connect_info ClientContext::makeConnectInfo(const Uri& uri) const
{
    lws_client_connect_info c_info;
    memset(&c_info, 0, sizeof(c_info));

    c_info.context = context;
    c_info.ietf_version_or_minus_one = -1;

    c_info.address = uri.host.c_str();
    c_info.port = uri.port ? uri.port : 80;
    c_info.path = uri.path.c_str();

    c_info.host = c_info.address;
    c_info.origin = lws_canonical_hostname(context);

    return c_info;
}

}
