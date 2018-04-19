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
#include "receiverImpl.h"
#include "utils.h"

namespace rockets
{
namespace jsonrpc
{
Receiver::Receiver()
    : _impl{std::make_unique<ReceiverImpl>()}
{
}

Receiver::~Receiver()
{
}

Receiver::Receiver(std::unique_ptr<RequestProcessor> impl)
    : _impl(std::move(impl))
{
}

void Receiver::connect(const std::string& method, VoidCallback action)
{
    bind(method, [action](const Request&) {
        action();
        return Response{"\"OK\""};
    });
}

void Receiver::connect(const std::string& method, NotifyCallback action)
{
    bind(method, [action](const Request& request) {
        action(request);
        return Response{"\"OK\""};
    });
}

void Receiver::bind(const std::string& method, ResponseCallback action)
{
    static_cast<ReceiverImpl*>(_impl.get())->registerMethod(method, action);
}

std::string Receiver::process(const Request& request)
{
    std::string result;
    _impl->process(request, [&result](std::string result_) {
        result = std::move(result_);
    });
    return result;
}
}
}
