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

#ifndef ROCKETS_JSONRPC_ASYNC_RECEIVER_H
#define ROCKETS_JSONRPC_ASYNC_RECEIVER_H

#include <rockets/jsonrpc/receiver.h>

#include <future>

namespace rockets
{
namespace jsonrpc
{
/**
 * Extends the synchronous receiver by providing asynchronous processing of
 * requests.
 */
class AsyncReceiver : public Receiver
{
public:
    /** Constructor. */
    AsyncReceiver();

    /**
     * Bind a method to an async response callback.
     *
     * @param method to register.
     * @param action to perform that will notify the caller upon completion.
     * @throw std::invalid_argument if the method name starts with "rpc."
     */
    void bindAsync(const std::string& method, DelayedResponseCallback action);

    /**
     * Bind a method to an async response callback with templated parameters.
     *
     * The parameters object must be deserializable by a free function:
     * from_json(Params& object, const std::string& json).
     *
     * @param method to register.
     * @param action to perform that will notify the caller upon completion.
     * @throw std::invalid_argument if the method name starts with "rpc."
     */
    template <typename Params>
    void bindAsync(const std::string& method,
                   std::function<void(Params, AsyncResponse)> action)
    {
        bindAsync(method,
                  [action](const Request& request, AsyncResponse callback) {
                      Params params;
                      if (from_json(params, request.message))
                          action(std::move(params), callback);
                      else
                          callback(Response::invalidParams());
                  });
    }

    /**
     * Process a JSON-RPC request asynchronously.
     *
     * @param request Request object with message in JSON-RPC 2.0 format.
     * @return future json response string in JSON-RPC 2.0 format.
     */
    std::future<std::string> processAsync(const Request& request);

    /**
     * Process a JSON-RPC request asynchronously.
     *
     * @param request Request object with message in JSON-RPC 2.0 format.
     * @param callback that return a json response string in JSON-RPC 2.0
     *        format upon request completion.
     */
    void process(const Request& request, AsyncStringResponse callback);

protected:
    AsyncReceiver(std::unique_ptr<RequestProcessor> impl);
};
}
}

#endif
