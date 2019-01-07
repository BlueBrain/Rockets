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

#pragma once

#include "requestProcessor.h"
#include "utils.h"

namespace rockets
{
namespace jsonrpc
{
class ReceiverImpl : public RequestProcessor
{
public:
    void registerMethod(const std::string& method, ResponseCallback action)
    {
        verifyValidMethodName(method);
        _methods[method] = action;
    }

    bool isRegisteredMethodName(const std::string& method) const override
    {
        return _methods.find(method) != _methods.end();
    }

    void process(const json& requestID, const std::string& method,
                 const Request& request, JsonResponseCallback respond) override
    {
        auto response = _methods[method](request);
        if (requestID.is_null())
            respond(json());
        else
            respond(makeResponse(response, requestID));
    }

private:
    std::map<std::string, ResponseCallback> _methods;
};
}
}
