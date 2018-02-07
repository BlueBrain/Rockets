/* Copyright (c) 2018, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
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

#ifndef ROCKETS_JSONRPC_ERROR_CODES_H
#define ROCKETS_JSONRPC_ERROR_CODES_H

#include <cstdint>

namespace rockets
{
namespace jsonrpc
{
/**
 * Error codes that can occur when making a request.
 */
enum ErrorCode : int16_t
{
    // JSON-RPC 2.0 specification; server errors [-32768 to -32000]
    parse_error = -32700,
    invalid_request = -32600,
    method_not_found = -32601,
    invalid_params = -32602,
    internal_error = -32603,
    // Rockets client errors
    invalid_json_response = -31001,
    request_aborted = -31002,
    http_error = -31003
};
}
}

#endif
