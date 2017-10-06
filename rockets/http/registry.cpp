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

#include "registry.h"

#include "../json.hpp"

#include <algorithm>

namespace rockets
{
namespace http
{
bool Registry::add(const Method method, const std::string& endpoint,
                   RESTFunc func)
{
    if (_methods[int(method)].count(endpoint) != 0)
        return false;

    _methods[int(method)][endpoint] = func;
    return true;
}

bool Registry::remove(const std::string& endpoint)
{
    bool foundMethod = false;
    for (auto& method : _methods)
        if (method.erase(endpoint) != 0)
            foundMethod = true;
    return foundMethod;
}

bool Registry::contains(const Method method, const std::string& endpoint) const
{
    const auto& funcMap = _methods[int(method)];
    return funcMap.count(endpoint);
}

RESTFunc Registry::getFunction(const Method method,
                               const std::string& endpoint) const
{
    const auto& funcMap = _methods[int(method)];
    return funcMap.at(endpoint);
}

std::string Registry::getAllowedMethods(const std::string& endpoint) const
{
    std::string methods;
    if (contains(Method::GET, endpoint))
        methods.append(methods.empty() ? "GET" : ", GET");
    if (contains(Method::POST, endpoint))
        methods.append(methods.empty() ? "POST" : ", POST");
    if (contains(Method::PUT, endpoint))
        methods.append(methods.empty() ? "PUT" : ", PUT");
    if (contains(Method::PATCH, endpoint))
        methods.append(methods.empty() ? "PATCH" : ", PATCH");
    if (contains(Method::DELETE, endpoint))
        methods.append(methods.empty() ? "DELETE" : ", DELETE");
    if (contains(Method::OPTIONS, endpoint))
        methods.append(methods.empty() ? "OPTIONS" : ", OPTIONS");
    return methods;
}

Registry::SearchResult Registry::findEndpoint(const Method method,
                                              const std::string& path) const
{
    const auto& funcMap = _methods[int(method)];
    auto it = _find(funcMap, path);
    if (it != funcMap.end())
        return {true, it->first};

    if (contains(method, "/")) // "/" should be passed all unhandled requests.
        return {true, "/"};

    return {false, std::string()};
}

Registry::FuncMap::const_iterator Registry::_find(
    const Registry::FuncMap& funcMap, const std::string& path) const
{
    const auto beginsWithPath = [&path](const FuncMap::value_type& pair) {
        const auto& endpoint = pair.first;
        return endpoint.empty() ? path.empty() : path.find(endpoint) == 0;
    };
    return std::find_if(funcMap.begin(), funcMap.end(), beginsWithPath);
}

void _append(nlohmann::json& array, std::string&& value)
{
    if (array.is_null())
        array = nlohmann::json::array({std::move(value)});
    else
        array.emplace_back(std::move(value));
}

std::string Registry::toJson() const
{
    auto body = nlohmann::json();
    for (const auto& i : _methods[int(Method::GET)])
        _append(body[i.first], "GET");
    for (const auto& i : _methods[int(Method::POST)])
        _append(body[i.first], "POST");
    for (const auto& i : _methods[int(Method::PUT)])
        _append(body[i.first], "PUT");
    for (const auto& i : _methods[int(Method::PATCH)])
        _append(body[i.first], "PATCH");
    for (const auto& i : _methods[int(Method::DELETE)])
        _append(body[i.first], "DELETE");
    for (const auto& i : _methods[int(Method::OPTIONS)])
        _append(body[i.first], "OPTIONS");
    return body.dump(4);
}
}
}
