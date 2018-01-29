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

#ifndef ROCKETS_JSONRPC_REQUESTER_H
#define ROCKETS_JSONRPC_REQUESTER_H

#include <rockets/jsonrpc/types.h>

#include <future>
#include <map>

namespace rockets
{
namespace jsonrpc
{
/**
 * Emitter of JSON-RPC requests.
 */
class Requester
{
public:
    virtual ~Requester() = default;

    /**
     * Make a request.
     *
     * @param method to call.
     * @param params for the request in json format (optional).
     * @return future result, including a possible error code.
     */
    std::future<Response> request(const std::string& method,
                                  const std::string& params);

    /**
     * Make a request.
     *
     * @param method to call.
     * @param params for the request in json format (optional).
     * @param callback to handle the result, including a possible error code.
     */
    void request(const std::string& method, const std::string& params,
                 AsyncResponse callback);

protected:
    /**
     * Process a JSON-RPC response, calling the associated callback.
     *
     * @param json response string in JSON-RPC 2.0 format.
     * @return false if the string is not a valid JSON-RPC response or no
     *         pending request matches the response id.
     */
    bool processResponse(const std::string& json);

private:
    virtual void _sendRequest(std::string json) = 0;

    std::map<size_t, AsyncResponse> pendingRequests;
    size_t lastId = 0u;
};
}
}

#endif
