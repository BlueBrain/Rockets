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

#ifndef ROCKETS_JSONRPC_REQUEST_PROCESSOR_H
#define ROCKETS_JSONRPC_REQUEST_PROCESSOR_H

#include <rockets/jsonrpc/types.h>

#include "../json.hpp"

namespace rockets
{
namespace jsonrpc
{
/**
 * JSON-RPC 2.0 request processor.
 *
 * See specification: http://www.jsonrpc.org/specification
 *
 * This class is designed to be used internally by the Receiver or AsyncReceiver
 * to handle processing of JSON-RPC messages, by using a JSON library which is
 * not meant to be exposed to the public interface.
 */
class RequestProcessor
{
public:
    virtual ~RequestProcessor() = default;

    /**
     * Process a JSON-RPC request asynchronously.
     *
     * @param request Request object with message in JSON-RPC 2.0 format.
     * @param callback that return a json response string in JSON-RPC 2.0
     *        format upon request completion.
     */
    void process(const Request& request, AsyncStringResponse callback);

    /** Check if given method name is valid, throws otherwise. */
    virtual void verifyValidMethodName(const std::string& method) const;

protected:
    using json = rockets_nlohmann::json;
    using JsonResponseCallback = std::function<void(json)>;

private:
    /**
     * Implements the processing of a valid JSON-RPC request. The minimum this
     * processing needs to do is to respond() the result of the request.
     *
     * @param requestID 'id' field of the JSON-RPC request
     * @param method 'method' field of the JSON-RPC request
     * @param request 'params' field of the JSON-RPC request and the client ID
     * @param respond callback for responding JSON result of request processing
     */
    virtual void process(const json& requestID, const std::string& method,
                         const Request& request,
                         JsonResponseCallback respond) = 0;

    /**
     * @return true if the given method name is valid to continue calling
     *         _process(), false otherwise
     */
    virtual bool isRegisteredMethodName(const std::string&) const = 0;

    std::string _processBatchBlocking(const json& array, uintptr_t clientID);

    json _processValidBatchBlocking(const json& array,
                                    const uintptr_t clientID);
    json _processCommandBlocking(const json& request, const uintptr_t clientID);
    void _processCommand(const json& request, const uintptr_t clientID,
                         JsonResponseCallback respond);
};
}
}

#endif
