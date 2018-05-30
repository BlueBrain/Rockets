/* Copyright (c) 2017-2018, EPFL/Blue Brain Project
 *                          Raphael.Dumusc@epfl.ch
 *                          Stefan.Eilemann@epfl.ch
 *                          Daniel.Nachbaur@epfl.ch
 *                          Pawel.Podhajski@epfl.ch
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
         const unsigned int threadCount, void* uvLoop)
        : handler{registry}
    {
        context =
            std::make_unique<ServerContext>(uri, name, threadCount,
                                            callback_http, callback_websockets,
                                            this, uvLoop);
        if (threadCount > 0)
            serviceThreadPool = std::make_unique<ServiceThreadPool>(*context);
    }

    void requestBroadcast()
    {
        if (serviceThreadPool)
            serviceThreadPool->requestBroadcast();
        else
            context->requestBroadcast();
    }

    void openWsConnection(lws* wsi)
    {
        std::lock_guard<std::mutex> lock{wsConnectionsMutex};
        wsConnections.emplace(wsi, std::make_shared<ws::Connection>(
                                       std::make_unique<ws::Channel>(wsi)));
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
    std::map<lws*, ws::ConnectionPtr> wsConnections;
    ws::MessageHandler wsHandler;

    PollDescriptors pollDescriptors;
    std::unique_ptr<ServerContext> context;
    std::unique_ptr<ServiceThreadPool> serviceThreadPool;
};

Server::Server(const std::string& uri, const std::string& name,
               const unsigned int threadCount)
    : _impl(new Impl(uri, name, threadCount, nullptr))
{
}

Server::Server(const unsigned int threadCount)
    : _impl(new Impl(std::string(), std::string(), threadCount, nullptr))
{
}

Server::Server(void* uvLoop, const std::string& uri, const std::string& name)
    : _impl(new Impl(uri, name, 0, uvLoop))
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

void Server::handleOpen(ws::ConnectionCallback callback)
{
    _impl->wsHandler.callbackOpen = callback;
}

void Server::handleClose(ws::ConnectionCallback callback)
{
    _impl->wsHandler.callbackClose = callback;
}

void Server::handleText(ws::MessageCallback callback)
{
    _impl->wsHandler.callbackText = callback;
}

void Server::handleText(ws::MessageCallbackAsync callback)
{
    _impl->wsHandler.callbackTextAsync = callback;
}

void Server::handleBinary(ws::MessageCallback callback)
{
    _impl->wsHandler.callbackBinary = callback;
}

void Server::broadcastText(const std::string& message)
{
    std::lock_guard<std::mutex> lock{_impl->wsConnectionsMutex};
    for (auto& connection : _impl->wsConnections)
        connection.second->enqueueText(message);
    _impl->requestBroadcast();
}

void Server::broadcastText(const std::string& message,
                           const std::set<uintptr_t>& filter)
{
    std::lock_guard<std::mutex> lock{_impl->wsConnectionsMutex};
    for (auto& connection : _impl->wsConnections)
    {
        auto i =
            filter.find(reinterpret_cast<uintptr_t>(connection.second.get()));
        if (i == filter.end())
            connection.second->enqueueText(message);
    }
    _impl->requestBroadcast();
}

void Server::sendText(const std::string& message, uintptr_t client)
{
    std::lock_guard<std::mutex> lock{_impl->wsConnectionsMutex};
    for (auto& connection : _impl->wsConnections)
    {
        if (client == reinterpret_cast<uintptr_t>(connection.second.get()))
            connection.second->enqueueText(message);
    }
    _impl->requestBroadcast();
}

void Server::broadcastBinary(const char* data, const size_t size)
{
    std::lock_guard<std::mutex> lock{_impl->wsConnectionsMutex};
    for (auto& connection : _impl->wsConnections)
        connection.second->enqueueBinary({data, size});
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
    if (_impl->serviceThreadPool)
        throw std::logic_error("No process() when using service threads");
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
        auto& connections = impl->connections;

        switch (reason)
        {
        case LWS_CALLBACK_HTTP:
            connections.emplace(wsi, http::Connection{wsi, (const char*)in});
            handler.handleNewRequest(connections.at(wsi));
            break;
        case LWS_CALLBACK_HTTP_BODY:
            handler.handleData(connections.at(wsi), (const char*)in, len);
            break;
        case LWS_CALLBACK_HTTP_BODY_COMPLETION:
            handler.prepareResponse(connections.at(wsi));
            break;

        case LWS_CALLBACK_HTTP_WRITEABLE:
            // A writable callback may exceptionally occcur without a connection
            if (connections.count(wsi))
                return handler.writeResponse(connections.at(wsi));
            break;

#if LWS_LIBRARY_VERSION_NUMBER >= 2001000
        case LWS_CALLBACK_HTTP_DROP_PROTOCOL: // fall-through
#endif
        case LWS_CALLBACK_CLOSED_HTTP:
            connections.erase(wsi);
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
            impl->wsHandler.handleOpenConnection(impl->wsConnections.at(wsi));
            break;
        case LWS_CALLBACK_CLOSED:
            impl->wsHandler.handleCloseConnection(impl->wsConnections.at(wsi));
            impl->closeWsConnection(wsi);
            break;
        case LWS_CALLBACK_RECEIVE:
            impl->wsHandler.handleMessage(impl->wsConnections.at(wsi),
                                          (const char*)in, len);
            break;
        case LWS_CALLBACK_SERVER_WRITEABLE:
            impl->wsConnections.at(wsi)->writeMessages();
            break;
        default:
            break;
        }
    }
    return 0;
}
}
