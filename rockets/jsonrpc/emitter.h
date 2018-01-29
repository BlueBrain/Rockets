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

#ifndef ROCKETS_JSONRPC_EMITTER_H
#define ROCKETS_JSONRPC_EMITTER_H

#include <string>

namespace rockets
{
namespace jsonrpc
{
/**
 * Emitter of JSON-RPC notifications (request without an id and no response).
 */
class Emitter
{
public:
    virtual ~Emitter() = default;

    /**
     * Emit a notification.
     *
     * @param method to call.
     * @param params for the notification.
     */
    void emit(const std::string& method, const std::string& params);

    /**
     * Emit a notification using a JSON-serializable object.
     *
     * @param method to call.
     * @param params object to send.
     */
    template <typename Params>
    void emit(const std::string& method, const Params& params)
    {
        emit(method, to_json(params));
    }

    /** @return a JSON RPC notification for the given method. */
    std::string makeNotification(const std::string& method) const;

    /** @return a JSON RPC notification for the given method and params. */
    std::string makeNotification(const std::string& method,
                                 const std::string& params) const;

    /**
     * @return a JSON RPC notification for the given method and templated
     *         params.
     */
    template <typename Params>
    std::string makeNotification(const std::string& method,
                                 const Params& params) const
    {
        return makeNotification(method, to_json(params));
    }

private:
    virtual void _sendNotification(std::string json) = 0;
};
}
}

#endif
