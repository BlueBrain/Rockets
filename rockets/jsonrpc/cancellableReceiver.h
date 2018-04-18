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

#ifndef ROCKETS_JSONRPC_CANCELLABLE_RECEIVER_H
#define ROCKETS_JSONRPC_CANCELLABLE_RECEIVER_H

#include <rockets/jsonrpc/asyncReceiver.h>

namespace rockets
{
namespace jsonrpc
{
/**
 * Extends the asynchronous receiver by providing cancelable requests, which
 * will provide progress updates during their execution.
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
 *
 * The progress notification will have the following payload:
 *
 * @code{.json}
 * {
 *   "jsonrpc": "2.0",
 *   "method": "progress",
 *   "params": {
 *     "id": <request_id>
 *     "amount": float
 *     "string": operation
 *   }
 * }
 * @endcode
 */
class CancellableReceiver : public AsyncReceiver
{
public:
    /** Constructor. */
    explicit CancellableReceiver(SendTextCallback sendTextCb);

    /**
     * Bind a cancellable method to an async response callback.
     *
     * @param method to register.
     * @param action to perform that will notify the caller upon completion.
     * @throw std::invalid_argument if the method name starts with "rpc." or
     *                              "cancel"
     */
    void bindAsync(const std::string& method,
                   CancellableResponseCallback action);

    /**
     * Bind a cancellable method to an async response callback with templated
     * parameters.
     *
     * The parameters object must be deserializable by a free function:
     * from_json(Params& object, const std::string& json).
     *
     * @param method to register.
     * @param action to perform that will notify the caller upon completion.
     * @throw std::invalid_argument if the method name starts with "rpc." or
     *                              "cancel"
     */
    template <typename Params>
    void bindAsync(
        const std::string& method,
        std::function<CancelRequestCallback(Params, uintptr_t, AsyncResponse,
                                            ProgressUpdateCallback)>
            action)
    {
        bindAsync(method,
                  [action](const Request& request, AsyncResponse callback,
                           ProgressUpdateCallback progressUpdate) {
                      Params params;
                      if (from_json(params, request.message))
                          return action(std::move(params), request.clientID,
                                        callback, progressUpdate);
                      callback(Response::invalidParams());
                      return CancelRequestCallback{};
                  });
    }
};
}
}

#endif
