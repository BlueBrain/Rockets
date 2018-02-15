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

#ifndef ROCKETS_JSONRPC_RECEIVER_H
#define ROCKETS_JSONRPC_RECEIVER_H

#include <rockets/jsonrpc/receiverInterface.h>

namespace rockets
{
namespace jsonrpc
{
/**
 * Default receiver for synchronous and non-cancellable asynchronous request
 * processing.
 */
class Receiver : public ReceiverInterface
{
public:
    /** Constructor. */
    Receiver();

    /** Destructor. */
    ~Receiver();

private:
    class Impl;
    std::unique_ptr<Impl> _impl;

    void _process(const Request& request, AsyncStringResponse callback) final;
    void _registerMethod(const std::string& method,
                         DelayedResponseCallback action) final;
};
}
}

#endif
