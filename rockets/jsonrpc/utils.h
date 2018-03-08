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

#ifndef ROCKETS_JSONRPC_UTILS_H
#define ROCKETS_JSONRPC_UTILS_H

#include <rockets/jsonrpc/response.h>

#include "../json.hpp"

namespace rockets
{
namespace jsonrpc
{
const std::string jsonResponseParseError =
    "Server response is not a valid json string";

const Response::Error internalError{"Internal error",
                                    ErrorCode::internal_error};

using json = rockets_nlohmann::json;

inline json makeErrorResponse(const json& error, const json& id)
{
    return json{{"jsonrpc", "2.0"}, {"error", error}, {"id", id}};
}

inline json makeErrorResponse(const Response::Error& error,
                              const json& id = json())
{
    return makeErrorResponse(json{{"code", error.code},
                                  {"message", error.message}},
                             id);
}

inline json makeErrorResponse(const Response::Error& error, const json& id,
                              const json& data)
{
    return makeErrorResponse(json{{"code", error.code},
                                  {"message", error.message},
                                  {"data", data}},
                             id);
}

inline json makeResponse(const json& result, const json& id)
{
    return json{{"jsonrpc", "2.0"}, {"result", result}, {"id", id}};
}

inline json makeResponse(const std::string& resultJson, const json& id)
{
    try
    {
        return makeResponse(json::parse(resultJson), id);
    }
    catch (const json::parse_error&)
    {
        return makeErrorResponse(internalError, id, jsonResponseParseError);
    }
}

inline json makeResponse(const Response& rep, const json& id)
{
    return rep.isError() ? makeErrorResponse(rep.error, id)
                         : makeResponse(rep.result, id);
}

inline bool begins_with(const std::string& string, const std::string& other)
{
    return string.compare(0, other.length(), other) == 0;
}
}
}

#endif
