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

#include "cancellableReceiver.h"
#include "cancellableReceiverImpl.h"
#include "helpers.h"
#include "utils.h"

namespace rockets
{
namespace jsonrpc
{
CancellableReceiver::CancellableReceiver(SendTextCallback sendTextCb)
    : AsyncReceiver{new CancellableReceiverImpl(sendTextCb)}
{
}

void CancellableReceiver::bindAsync(const std::string& method,
                                    CancellableResponseCallback action)
{
    std::static_pointer_cast<CancellableReceiverImpl>(_impl)->registerMethod(
        method, [action](Request request, AsyncResponse response,
                         ProgressUpdateCallback progress) {
            return action(request, response, progress);
        });
}
}
}
