/* Copyright (c) 2017, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
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

#include "serviceThreadPool.h"

#ifdef __linux__
#include <sys/prctl.h>
#endif

namespace
{
const auto serviceTimeoutMs = 50;

void setThreadName(const std::string& name)
{
#ifdef __APPLE__
    pthread_setname_np(name.c_str());
#elif defined(__linux__)
    prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
#endif
}
}

namespace rockets
{
ServiceThreadPool::ServiceThreadPool(ServerContext& context_)
    : context(context_)
    , broadcastRequested{new std::atomic_bool[context.getThreadCount()]}
{
    start();
}

ServiceThreadPool::~ServiceThreadPool()
{
    stop();
}

size_t ServiceThreadPool::getSize() const
{
    return serviceThreads.size();
}

void ServiceThreadPool::requestBroadcast()
{
    for (size_t tsi = 0; tsi < getSize(); ++tsi)
        broadcastRequested[tsi] = true;
}

void ServiceThreadPool::handleBroadcastRequest(const int tsi)
{
    if (broadcastRequested[tsi])
    {
        context.requestBroadcast();
        broadcastRequested[tsi] = false;
    }
}

void ServiceThreadPool::start()
{
    for (int tsi = 0; tsi < context.getThreadCount(); ++tsi)
    {
        const auto name = "rockets_" + std::to_string(tsi);
        serviceThreads.emplace_back(std::thread([this, tsi, name]() {
            setThreadName(name);
            while (context.service(tsi, serviceTimeoutMs) && !exitService)
                handleBroadcastRequest(tsi);
        }));
    }
}

void ServiceThreadPool::stop()
{
    exitService = true;
    context.cancelService();
    for (auto& thread : serviceThreads)
        thread.join();
}
}
