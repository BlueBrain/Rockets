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

#ifndef ROCKETS_JSONRPC_CLIENT_REQUEST_H
#define ROCKETS_JSONRPC_CLIENT_REQUEST_H

#include <rockets/helpers.h>
#include <rockets/jsonrpc/types.h>

#include <future>

namespace rockets
{
namespace jsonrpc
{
std::string _getCancelJson(size_t id);

/**
 * A request object containing the future result and a handle to the request
 * made by Requester to cancel the request, if supported by the receiver.
 */
template <typename ResponseT>
class ClientRequest
{
public:
    /** @return true if the result is ready and get() won't block. */
    bool is_ready() const { return rockets::is_ready(_future); }
    /** @return the result of the request, may wait if not ready yet. */
    ResponseT get() { return _future.get(); }
    /**
     * Issue a cancel of the request. Depending on the receiver type and the
     * internal cancel handling, is_ready() might change.
     */
    void cancel() { _notify("cancel", _getCancelJson(_id)); }
private:
    friend class Requester;

    using NotifyFunc = std::function<void(std::string, std::string)>;
    ClientRequest(const size_t id, std::future<ResponseT>&& future,
                  NotifyFunc func)
        : _id(id)
        , _future(std::move(future))
        , _notify(func)
    {
    }

    const size_t _id;
    std::future<ResponseT> _future;
    NotifyFunc _notify;
};
}
}

#endif
