/* Copyright (c) 2017-2018, EPFL/Blue Brain Project
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

#ifndef ROCKETS_JSONRPC_ASYNC_RECEIVER_H
#define ROCKETS_JSONRPC_ASYNC_RECEIVER_H

#include <rockets/jsonrpc/receiverInterface.h>

namespace rockets
{
namespace jsonrpc
{
/**
 * A receiver for synchronous and asynchronous request processing, where
 * asychronous requests can be cancelled with a reserved RPC.
 *
 * The cancel notification needs to have the following payload:
 *
 * @code{.json}
 * {
 *   "jsonrpc": "2.0",
 *   "method": "cancel",
 *   "params": { "id": <request_to_cancel> }
 * }
 * @endcode
 */
class AsyncReceiver : public ReceiverInterface
{
public:
    using CancelRequestCallback = std::function<void()>;

    /** Constructor. */
    AsyncReceiver();

    /**
     * Bind a cancellable method to an async response callback.
     *
     * @param method to register.
     * @param action to perform that will notify the caller upon completion.
     * @param cancel callback if request has been cancelled with the "cancel"
     *               notification.
     * @throw std::invalid_argument if the method name starts with "rpc." or
     *                              "cancel"
     */
    void bindAsync(const std::string& method, DelayedResponseCallback action,
                   CancelRequestCallback cancel);

    /**
     * Bind a cancellable method to an async response callback with templated
     * parameters.
     *
     * The parameters object must be deserializable by a free function:
     * from_json(Params& object, const std::string& json).
     *
     * @param method to register.
     * @param action to perform that will notify the caller upon completion.
     * @param cancel callback if request has been cancelled with the "cancel"
     *               notification.
     * @throw std::invalid_argument if the method name starts with "rpc." or
     *                              "cancel"
     */
    template <typename Params>
    void bindAsync(const std::string& method,
                   std::function<void(Params, AsyncResponse)> action,
                   CancelRequestCallback cancel)
    {
        bindAsync(method,
                  [action](const Request& request, AsyncResponse callback) {
                      Params params;
                      if (from_json(params, request.message))
                          action(std::move(params), callback);
                      else
                          callback(Response::invalidParams());
                  },
                  cancel);
    }

private:
    class Impl;
    std::shared_ptr<Impl> _impl;

    void _process(const Request& request, AsyncStringResponse callback) final;
    void _registerMethod(const std::string& method,
                         DelayedResponseCallback action) final;
};
}
}

#endif
