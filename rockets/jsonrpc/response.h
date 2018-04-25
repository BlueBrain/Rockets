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

#ifndef ROCKETS_JSONRPC_RESPONSE_H
#define ROCKETS_JSONRPC_RESPONSE_H

#include <string>

#include <rockets/jsonrpc/errorCodes.h>

namespace rockets
{
namespace jsonrpc
{
/**
 * Response to a well-formed RPC request.
 */
struct Response
{
    std::string result; // JSON encoded

    struct Error
    {
        std::string message; // short text description of the error
        int code = 0;
        std::string data{};
    } error;

    /**
     * Construct a successful response.
     * @param res The result of the request in JSON format.
     */
    Response(std::string&& res)
        : result{std::move(res)}
    {
    }

    /**
     * Construct an error response.
     * @param err The error that occured during the request.
     */
    Response(Error&& err)
        : error(std::move(err))
    {
    }

    bool isError() const { return error.code != 0; }
    static Response invalidParams()
    {
        return Response{Error{"Invalid params", ErrorCode::invalid_params}};
    }
};
}
}

#endif
