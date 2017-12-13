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

#include "client.h"

#include "../clientContext.h"
#include "../pollDescriptors.h"
#include "../proxyConnectionError.h"
#include "channel.h"
#include "connection.h"
#include "messageHandler.h"

namespace
{
const char* wsProtocolNotFound = "unsupported websocket protocol";
}

namespace rockets
{
namespace ws
{
static int callback_ws(lws* wsi, lws_callback_reasons reason, void* user,
                       void* in, const size_t len);

class Client::Impl
{
public:
    Impl()
        : context{new ClientContext{callback_ws, this}}
    {
    }

    void tryToSetConnectionException()
    {
        try
        {
            connectionPromise.set_exception(std::current_exception());
        }
        catch (const std::future_error&) // promise may already be set
        {
        }
    }

    PollDescriptors pollDescriptors;

    std::promise<void> connectionPromise;
    std::unique_ptr<Connection> connection;

    MessageHandler messageHandler;

    std::unique_ptr<ClientContext> context; // must be destructed first
};

Client::Client()
    : _impl(new Impl())
{
}

Client::~Client()
{
}

std::future<void> Client::connect(const std::string& uri,
                                  const std::string& protocol)
{
    try
    {
        _impl->connection = _impl->context->connect(uri, protocol);
    }
    catch (...)
    {
        _impl->tryToSetConnectionException();
    }
    return _impl->connectionPromise.get_future();
}

void Client::sendText(std::string message)
{
    _impl->connection->sendText(std::move(message));
}

void Client::handleText(MessageCallback callback)
{
    _impl->messageHandler.callbackText = callback;
}

void Client::handleBinary(MessageCallback callback)
{
    _impl->messageHandler.callbackBinary = callback;
}

void Client::sendBinary(const char* data, const size_t size)
{
    _impl->connection->sendBinary({data, size});
}

void Client::_setSocketListener(SocketListener* listener)
{
    _impl->pollDescriptors.setListener(listener);
}

void Client::_processSocket(const SocketDescriptor fd, const int events)
{
    try
    {
        _impl->context->service(_impl->pollDescriptors, fd, events);
    }
    catch (const proxy_connection_error&)
    {
        _impl->tryToSetConnectionException();
    }
}

void Client::_process(const int timeout_ms)
{
    try
    {
        _impl->context->service(timeout_ms);
    }
    catch (const proxy_connection_error&)
    {
        _impl->tryToSetConnectionException();
    }
}

static int callback_ws(lws* wsi, lws_callback_reasons reason, void* /*user*/,
                       void* in, const size_t len)
{
    // Protocol may be null during the initial callbacks
    if (auto protocol = lws_get_protocol(wsi))
    {
        auto client = static_cast<Client::Impl*>(protocol->user);
        switch (reason)
        {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            client->connectionPromise.set_value();
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        {
            auto msg = in ? std::string((char*)in, len) : wsProtocolNotFound;
            auto exception = std::make_exception_ptr(std::runtime_error(msg));
            client->connectionPromise.set_exception(exception);
            break;
        }

        case LWS_CALLBACK_CLIENT_RECEIVE:
            client->messageHandler.handleMessage(*client->connection,
                                                 (const char*)in, len);
            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            client->connection->writeMessages();
            break;

        case LWS_CALLBACK_ADD_POLL_FD:
            client->pollDescriptors.add(static_cast<lws_pollargs*>(in));
            break;
        case LWS_CALLBACK_DEL_POLL_FD:
            client->pollDescriptors.remove(static_cast<lws_pollargs*>(in));
            break;
        case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
            client->pollDescriptors.update(static_cast<lws_pollargs*>(in));
            break;
        default:
            break;
        }
    }
    return 0;
}
}
}
