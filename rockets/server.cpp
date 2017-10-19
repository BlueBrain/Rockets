/* Copyright (c) 2017, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
 *                     Stefan.Eilemann@epfl.ch
 *                     Daniel.Nachbaur@epfl.ch
 *                     Pawel.Podhajski@epfl.ch
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

#include "server.h"

#include "http/connection.h"
#include "http/connectionHandler.h"
#include "http/registry.h"
#include "pollDescriptors.h"
#include "serverContext.h"
#include "serviceThreadPool.h"
#include "ws/channel.h"
#include "ws/connection.h"
#include "ws/messageHandler.h"

#include <libwebsockets.h>

#include <mutex>
#include <set>
#include <sstream>
#include <vector>

namespace
{
const std::string REQUEST_REGISTRY = "registry";
}

namespace rockets
{
static int callback_http(lws* wsi, lws_callback_reasons reason, void* user,
                         void* in, const size_t len);
static int callback_websockets(lws* wsi, lws_callback_reasons reason,
                               void* user, void* in, const size_t len);

class Server::Impl
{
public:
    Impl(const std::string& uri, const std::string& name,
         const unsigned int threadCount)
        : handler{registry}
    {
        context =
            make_unique<ServerContext>(uri, name, threadCount, callback_http,
                                       callback_websockets, this);
        if (threadCount > 0)
            serviceThreadPool = make_unique<ServiceThreadPool>(*context);
    }

    void requestBroadcast()
    {
        if (serviceThreadPool)
            serviceThreadPool->requestBroadcast();
        else
            wsBroadcast = true;
    }

    void handleBroadcastRequest()
    {
        if (serviceThreadPool)
            return;

        context->requestBroadcast();
        wsBroadcast = false;
    }

    void openWsConnection(lws* wsi)
    {
        std::lock_guard<std::mutex> lock{wsConnectionsMutex};
        wsConnections.emplace(wsi,
                              ws::Connection{make_unique<ws::Channel>(wsi)});
    }

    void closeWsConnection(lws* wsi)
    {
        std::lock_guard<std::mutex> lock{wsConnectionsMutex};
        wsConnections.erase(wsi);
    }

    http::Registry registry;
    http::ConnectionHandler handler;
    std::map<lws*, http::Connection> connections;

    std::mutex wsConnectionsMutex;
    std::map<lws*, ws::Connection> wsConnections;
    bool wsBroadcast = false;
    ws::MessageHandler wsHandler;

    PollDescriptors pollDescriptors;
    std::unique_ptr<ServerContext> context;
    std::unique_ptr<ServiceThreadPool> serviceThreadPool;
};

Server::Server(const std::string& uri, const std::string& name,
               const unsigned int threadCount)
    : _impl(new Impl(uri, name, threadCount))
{
}

Server::Server(const unsigned int threadCount)
    : _impl(new Impl(std::string(), std::string(), threadCount))
{
}

Server::~Server()
{
}

std::string Server::getURI() const
{
    const auto host = _impl->context->getHostname();

    std::stringstream ss;
    ss << (host.empty() ? "localhost" : host) << ":" << getPort();
    return ss.str();
}

uint16_t Server::getPort() const
{
    return _impl->context->getPort();
}

unsigned int Server::getThreadCount() const
{
    return _impl->serviceThreadPool ? _impl->serviceThreadPool->getSize() : 0;
}

void Server::setHttpFilter(const http::Filter* filter)
{
    _impl->handler.setFilter(filter);
}

bool Server::handle(const http::Method action, const std::string& endpoint,
                    http::RESTFunc func)
{
    if (endpoint == REQUEST_REGISTRY)
        throw std::invalid_argument("'registry' is a reserved endpoint");

    return _impl->registry.add(action, endpoint, func);
}

bool Server::remove(const std::string& endpoint)
{
    return _impl->registry.remove(endpoint);
}

void Server::handleText(ws::MessageCallback callback)
{
    _impl->wsHandler.callbackText = callback;
}

void Server::handleBinary(ws::MessageCallback callback)
{
    _impl->wsHandler.callbackBinary = callback;
}

void Server::broadcastText(const std::string& message)
{
    std::lock_guard<std::mutex> lock{_impl->wsConnectionsMutex};
    for (auto& connection : _impl->wsConnections)
        connection.second.enqueueText(message);
    _impl->requestBroadcast();
}

void Server::broadcastBinary(const char* data, const size_t size)
{
    std::lock_guard<std::mutex> lock{_impl->wsConnectionsMutex};
    for (auto& connection : _impl->wsConnections)
        connection.second.enqueueBinary({data, size});
    _impl->requestBroadcast();
}

size_t Server::getConnectionCount() const
{
    std::lock_guard<std::mutex> lock{_impl->wsConnectionsMutex};
    return _impl->wsConnections.size();
}

void Server::_setSocketListener(SocketListener* listener)
{
    _impl->pollDescriptors.setListener(listener);
}

void Server::_processSocket(const SocketDescriptor fd, const int events)
{
    _impl->context->service(_impl->pollDescriptors, fd, events);
}

void Server::_process(const int timeout_ms)
{
    _impl->handleBroadcastRequest();
    _impl->context->service(timeout_ms);
}

static int callback_http(lws* wsi, const lws_callback_reasons reason,
                         void* /*user*/, void* in, const size_t len)
{
    // Protocol may be null during the initial callbacks
    if (auto protocol = lws_get_protocol(wsi))
    {
        auto impl = static_cast<Server::Impl*>(protocol->user);
        const auto& handler = impl->handler;

        switch (reason)
        {
        case LWS_CALLBACK_HTTP:
            impl->connections.emplace(wsi,
                                      http::Connection{wsi, (const char*)in});
            return handler.handleNewRequest(impl->connections.at(wsi));
        case LWS_CALLBACK_HTTP_BODY:
            return handler.handleData(impl->connections.at(wsi),
                                      (const char*)in, len);
        case LWS_CALLBACK_HTTP_BODY_COMPLETION:
            return handler.respondToRequest(impl->connections.at(wsi));

        case LWS_CALLBACK_HTTP_WRITEABLE:
            if (impl->connections.count(wsi))
                return handler.respondToRequest(impl->connections.at(wsi));
            break;

        case LWS_CALLBACK_HTTP_DROP_PROTOCOL: // nobreak
        case LWS_CALLBACK_CLOSED_HTTP:
            impl->connections.erase(wsi);
            break;

        case LWS_CALLBACK_ADD_POLL_FD:
            impl->pollDescriptors.add(static_cast<lws_pollargs*>(in));
            break;
        case LWS_CALLBACK_DEL_POLL_FD:
            impl->pollDescriptors.remove(static_cast<lws_pollargs*>(in));
            break;
        case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
            impl->pollDescriptors.update(static_cast<lws_pollargs*>(in));
            break;
        default:
            break;
        }
    }
    return 0;
}

static int callback_websockets(lws* wsi, const lws_callback_reasons reason,
                               void* /*user*/, void* in, const size_t len)
{
    // Protocol may be null during the initial callbacks
    if (auto protocol = lws_get_protocol(wsi))
    {
        auto impl = static_cast<Server::Impl*>(protocol->user);

        switch (reason)
        {
        case LWS_CALLBACK_ESTABLISHED:
            impl->openWsConnection(wsi);
            break;
        case LWS_CALLBACK_CLOSED:
            impl->closeWsConnection(wsi);
            break;

        case LWS_CALLBACK_RECEIVE:
        {
            const auto format = ws::Channel{wsi}.getCurrentMessageFormat();
            auto& connection = impl->wsConnections.at(wsi);
            impl->wsHandler.handle(connection, (const char*)in, len, format);
            break;
        }
        case LWS_CALLBACK_SERVER_WRITEABLE:
            impl->wsConnections.at(wsi).writeMessages();
            break;
        default:
            break;
        }
    }
    return 0;
}
}
