/* Copyright (c) 2018, EPFL/Blue Brain Project
 *                     Daniel.Nachbaur@epfl.ch
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

#ifndef ROCKETS_RESPONSEERROR_H
#define ROCKETS_RESPONSEERROR_H

#include <stdexcept>

namespace rockets
{
namespace jsonrpc
{
class response_error : public std::runtime_error
{
public:
    response_error(const std::string& what, const int code_)
        : std::runtime_error(what)
        , code(code_)
    {}

    const int code;
};
}
}

#endif
