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

#include "utils.h"

#include <lws_config.h>

// for NI_MAXHOST
#ifdef _WIN32
#include <Ws2tcpip.h>
#else
#include <netdb.h>
#include <unistd.h>
// For getIP
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#endif

#include <vector>

namespace rockets
{
Uri parse(const std::string& uri)
{
    // uri is modified by lws_parse_uri(), make a copy.
    // WAR: pre-C++11 stdlib has copy-on-write semantics for strings, make sure
    // the string is REALLY copied by using the specific constructor below:
    auto copy = std::string(uri.data(), uri.length());
    auto in = const_cast<char*>(copy.c_str());
    const char* protocol = nullptr;
    const char* address = nullptr;
    int port = 0;
    const char* path = nullptr;
    if (lws_parse_uri(in, &protocol, &address, &port, &path))
        throw std::invalid_argument("invalid uri");
    if (port < 0 || port > UINT16_MAX)
        throw std::invalid_argument("uri has invalid port range");
    return {protocol, address, (uint16_t)port, std::string("/").append(path)};
}

lws_protocols make_protocol(const char* name, lws_callback_function* callback,
                            void* user)
{
    // clang-format off
    const size_t rx_buffer_size = 1048576; // 1MB
    return lws_protocols{ name, callback, 0, rx_buffer_size, 0, user
        #if LWS_LIBRARY_VERSION_NUMBER >= 2003000
                , 0
        #endif
    };
    // clang-format on
}

lws_protocols null_protocol()
{
    return make_protocol(nullptr, nullptr, nullptr);
}

namespace
{
bool _isValidInterface(ifaddrs* interface)
{
    if (!interface->ifa_addr)
        return false;
    switch (interface->ifa_addr->sa_family)
    {
    case AF_INET:
    case AF_INET6:
        return true;
    default:
        return false;
    }
}
socklen_t _getSocketAddressLength(ifaddrs* interface)
{
    switch (interface->ifa_addr->sa_family)
    {
    case AF_INET:
        return sizeof(sockaddr_in);
    case AF_INET6:
        return sizeof(sockaddr_in6);
    default:
        throw std::logic_error("No IPv4/6 in your universe?!");
    }
}
size_t _getIPAddressLength(ifaddrs* interface)
{
    switch (interface->ifa_addr->sa_family)
    {
    case AF_INET:
        return INET_ADDRSTRLEN;
    case AF_INET6:
        return INET6_ADDRSTRLEN;
    default:
        throw std::logic_error("No IPv4/6 in your universe?!");
    }
}
}

std::string getIP(const std::string& iface)
{
#ifdef _WIN32
    return std::string();
#else
    ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1)
        return std::string();

    char host[NI_MAXHOST] = {'\0'};
    for (auto ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (!_isValidInterface(ifa))
            continue;
        if (iface != ifa->ifa_name)
            continue;

        if (getnameinfo(ifa->ifa_addr, _getSocketAddressLength(ifa), host,
                        NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0)
        {
            // found it
            break;
        }
    }
    freeifaddrs(ifaddr);
    return host;
#endif
}

std::string getInterface(const std::string& hostnameOrIP)
{
    if (hostnameOrIP.empty())
        return std::string();
#ifdef _WIN32
    return std::string();
#else
    ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1)
        return std::string();

    std::string iface;
    for (auto ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (!_isValidInterface(ifa))
            continue;

        auto socketAddress = (sockaddr_in*)ifa->ifa_addr;
        std::vector<char> ip(_getIPAddressLength(ifa));
        inet_ntop(ifa->ifa_addr->sa_family, (void*)&socketAddress->sin_addr,
                  ip.data(), ip.size());

        char host[NI_MAXHOST] = {'\0'};
        getnameinfo(ifa->ifa_addr, _getSocketAddressLength(ifa), host,
                    NI_MAXHOST, NULL, 0, NI_NAMEREQD);

        if (hostnameOrIP == ip.data() || hostnameOrIP == host)
        {
            iface = ifa->ifa_name;
            break;
        }
    }
    freeifaddrs(ifaddr);
    return iface;
#endif
}

std::string getHostname()
{
    char host[NI_MAXHOST] = {'\0'};
    gethostname(host, NI_MAXHOST);
    host[NI_MAXHOST - 1] = '\0';
    return host;
}
}
