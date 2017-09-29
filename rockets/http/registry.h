/* Copyright (c) 2017, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
 *                     Stefan.Eilemann@epfl.ch
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

#ifndef ROCKETS_HTTP_REGISTRY_H
#define ROCKETS_HTTP_REGISTRY_H

#include "types.h"

#include <array>
#include <map>
#include <string>

namespace rockets
{
namespace http
{
/**
 * Registry for HTTP endpoints.
 */
class Registry
{
public:
    bool add(Method method, const std::string& endpoint, RESTFunc func);
    bool remove(const std::string& endpoint);

    bool contains(Method method, const std::string& endpoint) const;
    RESTFunc getFunction(Method method, const std::string& endpoint) const;

    std::string getAllowedMethods(const std::string& endpoint) const;

    struct SearchResult
    {
        bool found;
        std::string endpoint;
    };
    SearchResult findEndpoint(Method method, const std::string& path) const;

    std::string toJson() const;

private:
    // key stores endpoints of Serializable objects lower-case, hyphenated,
    // with '/' separators
    // must be an ordered map in order to iterate from the most specific path
    using FuncMap = std::map<std::string, RESTFunc, std::greater<std::string>>;
    std::array<FuncMap, size_t(Method::ALL)> _methods;

    FuncMap::const_iterator _find(const Registry::FuncMap& FuncMap,
                                  const std::string& path) const;
};
}
}

#endif
