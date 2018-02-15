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

#include "receiverInterface.h"
#include "utils.h"

namespace rockets
{
namespace jsonrpc
{
namespace
{
const std::string reservedMethodPrefix = "rpc.";
const char* reservedMethodError =
    "Method names starting with 'rpc.' are "
    "reserved by the standard / forbidden.";
}

ReceiverInterface::ReceiverInterface()
{
}

ReceiverInterface::~ReceiverInterface()
{
}

void ReceiverInterface::connect(const std::string& method, VoidCallback action)
{
    bind(method, [action](const Request&) {
        action();
        return Response{"\"OK\""};
    });
}

void ReceiverInterface::connect(const std::string& method,
                                NotifyCallback action)
{
    bind(method, [action](const Request& request) {
        action(request);
        return Response{"\"OK\""};
    });
}

void ReceiverInterface::bind(const std::string& method, ResponseCallback action)
{
    bindAsync(method,
              [this, action](const Request& req, AsyncResponse callback) {
                  callback(action(req));
              });
}

void ReceiverInterface::bindAsync(const std::string& method,
                                  DelayedResponseCallback action)
{
    if (begins_with(method, reservedMethodPrefix))
        throw std::invalid_argument(reservedMethodError);

    _registerMethod(method, action);
}

std::string ReceiverInterface::process(const Request& request)
{
    return processAsync(request).get();
}

std::future<std::string> ReceiverInterface::processAsync(const Request& request)
{
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();
    auto callback = [promise](std::string response) {
        promise->set_value(std::move(response));
    };
    process(request, callback);
    return future;
}

void ReceiverInterface::process(const Request& request,
                                AsyncStringResponse callback)
{
    _process(request, callback);
}
}
}
