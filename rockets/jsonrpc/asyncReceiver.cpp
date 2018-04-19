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

#include "asyncReceiver.h"
#include "asyncReceiverImpl.h"
#include "utils.h"

namespace rockets
{
namespace jsonrpc
{
AsyncReceiver::AsyncReceiver()
    : Receiver(std::make_unique<AsyncReceiverImpl>())
{
}

AsyncReceiver::AsyncReceiver(std::unique_ptr<RequestProcessor> impl)
    : Receiver(std::move(impl))
{
}

void AsyncReceiver::bindAsync(const std::string& method,
                              DelayedResponseCallback action)
{
    static_cast<AsyncReceiverImpl*>(_impl.get())
        ->registerMethod(method,
                         [action](Request request, AsyncResponse response) {
                             action(request, response);
                         });
}

std::future<std::string> AsyncReceiver::processAsync(const Request& request)
{
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();
    auto callback = [promise](std::string response) {
        promise->set_value(std::move(response));
    };
    process(request, callback);
    return future;
}

void AsyncReceiver::process(const Request& request,
                            AsyncStringResponse callback)
{
    _impl->process(request, callback);
}
}
}
