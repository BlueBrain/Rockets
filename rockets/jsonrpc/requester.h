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

#include <rockets/jsonrpc/clientRequest.h>
#include <rockets/jsonrpc/notifier.h>
#include <rockets/jsonrpc/responseError.h>
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
class Requester : public Notifier
{
public:
    Requester();
    ~Requester();

    /**
     * Make a request.
     *
     * @param method to call.
     * @param params for the request in json format (optional).
     * @return request with future result, can contain a possible error code,
     *         but not throw.
     */
    ClientRequest<Response> request(const std::string& method,
                                    const std::string& params);

    /**
     * Make a request.
     *
     * @param method to call.
     * @param params for the request in json format (optional).
     * @param callback to handle the result, including a possible error code.
     * @return ID of the request
     */
    size_t request(const std::string& method, const std::string& params,
                   AsyncResponse callback);

    /**
     * Make a request with templated parameters and result.
     *
     * @param method to call.
     * @param params for the request, must be serializable to JSON with
     *               `std::string to_json(const Params&)`
     * @return Request containing the result of the request, must be
     *         serializable from JSON with
     *         `bool from_json(RetVal& obj, const std::string& json)`
     * @throw response_error if request returns error or from_json on RetVal
     * failed
     */
    template <typename Params, typename RetVal>
    ClientRequest<RetVal> request(const std::string& method,
                                  const Params& params)
    {
        auto promise = std::make_shared<std::promise<RetVal>>();
        auto callback = [promise](const Response& response) {
            if (response.isError())
                promise->set_exception(
                    std::make_exception_ptr(response_error(response.error)));
            else
            {
                RetVal value;
                if (!from_json(value, response.result))
                    promise->set_exception(jsonConversionFailed());
                else
                    promise->set_value(std::move(value));
            }
        };
        return {request(method, to_json(params), callback),
                promise->get_future(),
                [this](std::string method_, std::string params_) {
                    notify(method_, params_);
                }};
    }

    /**
     * Make a request with no parameters, but a typed result.
     *
     * @param method to call.
     * @return Request containing the result of the request, must be
     *         serializable from JSON with
     *         `bool from_json(RetVal& obj, const std::string& json)`
     * @throw response_error if request returns error or from_json on RetVal
     * failed
     */
    template <typename RetVal>
    ClientRequest<RetVal> request(const std::string& method)
    {
        auto promise = std::make_shared<std::promise<RetVal>>();
        auto callback = [promise](const Response& response) {
            if (response.isError())
                promise->set_exception(
                    std::make_exception_ptr(response_error(response.error)));
            else
            {
                RetVal value;
                if (!from_json(value, response.result))
                    promise->set_exception(jsonConversionFailed());
                else
                    promise->set_value(std::move(value));
            }
        };
        return {request(method, "", callback), promise->get_future(),
                [this](std::string method_, std::string params_) {
                    notify(method_, params_);
                }};
    }

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
    static std::exception_ptr jsonConversionFailed();

    class Impl;
    std::unique_ptr<Impl> _impl;
};
}
}

#endif
