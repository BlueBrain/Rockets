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

#ifndef ROCKETS_JSONRPC_RECEIVER_H
#define ROCKETS_JSONRPC_RECEIVER_H

#include <rockets/jsonrpc/responseError.h>
#include <rockets/jsonrpc/types.h>

#include <memory>

namespace rockets
{
namespace jsonrpc
{
/**
 * A base class for receiver implementations which provides synchronous
 * processing of JSON-RPC requests.
 */
class Receiver
{
public:
    /** Constructor. */
    Receiver();

    /** Destructor. */
    virtual ~Receiver();

    /**
     * Connect a method to a callback with no response and no payload.
     *
     * This is a convienience function that replies with a default "OK" response
     * if the caller asks for one (jsonrpc request id).
     *
     * @param method to register.
     * @param action to perform.
     * @throw std::invalid_argument if the method name starts with "rpc."
     */
    void connect(const std::string& method, VoidCallback action);

    /**
     * Connect a method to a callback with no response.
     *
     * This is a convienience function that replies with a default "OK" response
     * if the caller asks for one (jsonrpc request id).
     *
     * @param method to register.
     * @param action to perform.
     * @throw std::invalid_argument if the method name starts with "rpc."
     */
    void connect(const std::string& method, NotifyCallback action);

    /**
     * Connect a method to a callback with request parameters but no response.
     *
     * The templated Params object must be deserializable by a free function:
     * from_json(Params& object, const std::string& json).
     *
     * @param method to register.
     * @param action to perform.
     * @throw std::invalid_argument if the method name starts with "rpc."
     */
    template <typename Params>
    void connect(const std::string& method, std::function<void(Params)> action)
    {
        bind(method, [action](const Request& request) {
            Params params;
            if (!from_json(params, request.message))
                return Response::invalidParams();
            action(std::move(params));
            return Response{"\"OK\""};
        });
    }

    /**
     * Bind a method to a response callback.
     *
     * @param method to register.
     * @param action to perform.
     * @throw std::invalid_argument if the method name starts with "rpc."
     */
    void bind(const std::string& method, ResponseCallback action);

    /**
     * Bind a method to a response callback with templated request parameters.
     *
     * The parameters object must be deserializable by a free function:
     * from_json(Params& object, const std::string& json).
     *
     * @param method to register.
     * @param action to perform.
     * @throw std::invalid_argument if the method name starts with "rpc."
     */
    template <typename Params>
    void bind(const std::string& method, std::function<Response(Params)> action)
    {
        bind(method, [action](const Request& request) {
            Params params;
            if (!from_json(params, request.message))
                return Response::invalidParams();
            return action(std::move(params));
        });
    }

    /**
     * Bind a method to a response callback with templated request parameters
     * and templated return type. The response callback can throw an exception
     * of type response_error to indicate errors.
     *
     * The parameters object must be deserializable by a free function:
     * from_json(Params& object, const std::string& json). The return type must
     * be serializable by a free function: std::string to_json(const RetVal&)
     *
     * @param method to register.
     * @param action to perform.
     * @throw std::invalid_argument if the method name starts with "rpc."
     */
    template <typename Params, typename RetVal>
    void bind(const std::string& method, std::function<RetVal(Params)> action)
    {
        bind(method, [action](const Request& request) {
            Params params;
            if (!from_json(params, request.message))
                return Response::invalidParams();
            try
            {
                const auto& ret = action(std::move(params));
                return Response{to_json(ret)};
            }
            catch (const response_error& e)
            {
                return Response{Response::Error{e.what(), e.code}};
            }
        });
    }

    /**
     * Bind a method to a callback with no parameters but a response.
     *
     * The return type must be serializable by a free function:
     * std::string to_json(const RetVal&)
     *
     * @param method to register.
     * @param action to perform.
     * @throw std::invalid_argument if the method name starts with "rpc."
     */
    template <typename RetVal>
    void bind(const std::string& method, std::function<RetVal()> action)
    {
        bind(method, [action](const Request&) {
            try
            {
                const auto& ret = action();
                return Response{to_json(ret)};
            }
            catch (const response_error& e)
            {
                return Response{Response::Error{e.what(), e.code}};
            }
        });
    }

    /**
     * Process a JSON-RPC request and block for the result.
     *
     * @param request Request object with message in JSON-RPC 2.0 format.
     * @return json response string in JSON-RPC 2.0 format.
     */
    std::string process(const Request& request);

protected:
    Receiver(std::unique_ptr<RequestProcessor> impl);
    std::unique_ptr<RequestProcessor> _impl;
};
}
}

#endif
