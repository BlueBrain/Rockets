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
#include "../utils.h"
#include "channel.h"
#include "requestHandler.h"
#include "utils.h"

#include <libwebsockets.h>

namespace
{
#if LWS_LIBRARY_VERSION_NUMBER < 2001000
const char* bodyNotSupported = "Request body not supported with lws < 2.1";
#endif
const char* connectionFailure = "connection failed to start";
const int closeConnection = -1;

bool _isNotARealConnectionError(const std::string& message)
{
#if LWS_LIBRARY_VERSION_NUMBER >= 2003000
    return message == "HS: Server unrecognized response code";
#else
    return message == "HS: Server did not return 200 or 304";
#endif
}
} // anonymous namespace

namespace rockets
{
namespace http
{
static int callback_http(lws* wsi, lws_callback_reasons reason, void* user,
                         void* in, const size_t len);

class Client::Impl
{
public:
    Impl() { context = make_unique<ClientContext>(callback_http, this); }
    ~Impl()
    {
        abortPendingRequests();
        context.reset();
    }

    std::future<http::Response> startRequest(const http::Method method,
                                             const std::string& uri,
                                             std::string body)
    {
#if LWS_LIBRARY_VERSION_NUMBER < 2001000
        if (!body.empty())
            throw std::invalid_argument(bodyNotSupported);
#endif
        if (auto lws = context->startHttpRequest(method, uri))
        {
            requests.emplace(lws, http::RequestHandler{http::Channel{lws},
                                                       std::move(body)});
            return requests.at(lws).getFuture();
        }
        return http::make_error_future<std::runtime_error>(connectionFailure);
    }

    http::RequestHandler* getRequest(lws* wsi)
    {
        auto it = requests.find(wsi);
        return (it != requests.end()) ? &it->second : nullptr;
    }

    void finishRequest(lws* wsi)
    {
        if (auto request = getRequest(wsi))
            request->finish();
        requests.erase(wsi);
    }

    void abortRequest(lws* wsi, const std::string& reason = std::string())
    {
        if (auto request = getRequest(wsi))
        {
            auto message = std::string("connection failed");
            if (!reason.empty())
                message.append(": ").append(reason);
            request->abort(std::runtime_error{message});
        }
        requests.erase(wsi);
    }

    void abortPendingRequests()
    {
        for (auto& it : requests)
            it.second.abort(std::runtime_error{"client shutdown"});
        requests.clear();
    }

    std::unique_ptr<ClientContext> context;
    PollDescriptors pollDescriptors;
    std::map<lws*, http::RequestHandler> requests;
};

Client::Client()
    : _impl(new Impl())
{
}

Client::~Client()
{
}

std::future<http::Response> Client::request(const std::string& uri,
                                            const http::Method method,
                                            std::string body)
{
    return _impl->startRequest(method, uri, std::move(body));
}

void Client::_setSocketListener(SocketListener* listener)
{
    _impl->pollDescriptors.setListener(listener);
}

void Client::_processSocket(const SocketDescriptor fd, const int events)
{
    _impl->context->service(_impl->pollDescriptors, fd, events);
}

void Client::_process(const int timeout_ms)
{
    _impl->context->service(timeout_ms);
}

static int callback_http(lws* wsi, lws_callback_reasons reason, void* /*user*/,
                         void* in, const size_t len)
{
    // Protocol may be null during the initial callbacks
    if (auto protocol = lws_get_protocol(wsi))
    {
        auto client = static_cast<Client::Impl*>(protocol->user);
        auto request = client->getRequest(wsi);

        switch (reason)
        {
        case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
            return request->writeHeaders((unsigned char**)in, len);
#if LWS_LIBRARY_VERSION_NUMBER >= 2000000
        case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
            return request->writeBody();
        case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
            client->abortRequest(wsi);
            break;
#endif
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        {
            const auto message = std::string(in ? (const char*)in : "");
            if (_isNotARealConnectionError(message))
            {
                if (request)
                    request->readResponseHeaders();
                client->finishRequest(wsi);
            }
            else
                client->abortRequest(wsi, message);
            return closeConnection;
        }
#if LWS_LIBRARY_VERSION_NUMBER >= 2000000
        case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
            if (request)
            {
                request->readResponseHeaders();
                if (!request->hasResponseBody())
                {
                    client->finishRequest(wsi);
                    return closeConnection;
                }
            }
            break;
        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
        {
            /* In the case of chunked content, this will call back
             * LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ once per chunk or partial
             * chunk in the buffer, and report zero length back here.
             */
            char buffer[1024 + LWS_PRE];
            char* bufferPtr = buffer + LWS_PRE;
            int bufferSize = sizeof(buffer) - LWS_PRE;
            if (lws_http_client_read(wsi, &bufferPtr, &bufferSize) < 0)
                return closeConnection;
            break;
        }
        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
            request->appendToResponseBody((const char*)in, len);
            break;
        case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
            client->finishRequest(wsi);
            break;
#endif

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
