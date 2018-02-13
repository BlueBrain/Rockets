/* Copyright (c) 2018, EPFL/Blue Brain Project
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

#ifndef ROCKETS_JSONRPC_HTTP_H
#define ROCKETS_JSONRPC_HTTP_H

#include <rockets/http/client.h>
#include <rockets/http/types.h>
#include <rockets/jsonrpc/client.h>
#include <rockets/server.h>

namespace rockets
{
namespace jsonrpc
{
/**
 * Connect HTTP POST requests to a JSON-RPC receiver.
 *
 * @param server HTTP server that will be receiving POST requests.
 * @param endpoint the endpoint at which JSON-RPC requests will arrive.
 * @param receiver the JSON-RPC request processor.
 */
template <typename ReceiverT>
void connect(rockets::Server& server, const std::string& endpoint,
             ReceiverT& receiver)
{
    auto processJsonRpc = [&receiver](const http::Request& request) {
        // Package json-rpc response in http::Response when ready
        auto promise = std::make_shared<std::promise<http::Response>>();
        auto callback = [promise](const std::string& body) {
            if (body.empty())
                promise->set_value(http::Response{http::Code::OK});
            else
                promise->set_value(
                    http::Response{http::Code::OK, body, "application/json"});
        };
        receiver.process(request.body, callback);
        return promise->get_future();
    };
    server.handle(http::Method::POST, endpoint, processJsonRpc);
}

/**
 * Adapter for jsonrpc::Client to make JSON-RPC requests over an http::Client.
 *
 * Example usage:
 * http::Client httpClient;
 * HttpCommunicator communicator{httpClient, "localhost:8888/jsonrpc"};
 * Client<HttpCommunicator> client{communicator};
 * auto response = client.request(...);
 */
class HttpCommunicator
{
public:
    /**
     * Create a communicator for making requests to a specified url.
     * @param client_ an http client for performing the requests.
     * @param url_ the full path to the remote JSON-RPC HTTP endpoint.
     */
    HttpCommunicator(http::Client& client_, const std::string& url_)
        : client{client_}
        , url{url_}
    {
    }

private:
    http::Client& client;
    std::string url;
    ws::MessageCallback callback;

    friend class rockets::jsonrpc::Client<HttpCommunicator>;

    void sendText(std::string message);
    void handleText(ws::MessageCallback callback_) { callback = callback_; }
};
}
}

#endif
