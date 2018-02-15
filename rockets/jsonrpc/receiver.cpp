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

#include "receiver.h"
#include "requestProcessor.h"
#include "utils.h"

namespace rockets
{
namespace jsonrpc
{
class Receiver::Impl : public RequestProcessor
{
public:
    bool _isValidMethodName(const std::string& method) const final
    {
        return _methods.find(method) != _methods.end();
    }

    void _registerMethod(const std::string& method,
                         DelayedResponseCallback action)
    {
        _methods[method] = action;
    }

    void _process(const json& requestID, const std::string& method,
                  const Request& request, JsonResponseCallback respond) final
    {
        const auto& func = _methods[method];
        func(request, [respond, requestID](const Response rep) {
            // No reply for valid "notifications" (requests without an "id")
            if (requestID.is_null())
                respond(json());
            else
                respond(makeResponse(rep, requestID));
        });
    }

private:
    std::map<std::string, DelayedResponseCallback> _methods;
};

Receiver::Receiver()
    : _impl{new Impl}
{
}

Receiver::~Receiver()
{
}

void Receiver::_process(const Request& request, AsyncStringResponse callback)
{
    _impl->process(request, callback);
}

void Receiver::_registerMethod(const std::string& method,
                               DelayedResponseCallback action)
{
    _impl->_registerMethod(method, action);
}
}
}
