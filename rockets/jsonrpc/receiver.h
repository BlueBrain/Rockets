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

#include <future>
#include <memory>

namespace rockets
{
namespace jsonrpc
{
/**
 * JSON-RPC 2.0 request processor.
 *
 * See specification: http://www.jsonrpc.org/specification
 */
class Receiver
{
public:
    /** @name Callbacks that can be registered. */
    //@{
    using VoidCallback = std::function<void()>;
    using NotifyCallback = std::function<void(Request)>;
    using ResponseCallback = std::function<Response(Request)>;
    using DelayedResponseCallback =
        std::function<void(Request, AsyncResponse)>;
    //@}

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
                return Response{e.what(), e.code};
            }
        });
    }

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
     * Process a JSON-RPC request and block for the result.
     *
     * @param request Request object with message in JSON-RPC 2.0 format.
     * @return json response string in JSON-RPC 2.0 format.
     */
    std::string process(const Request& request);

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

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
}
}

#endif
