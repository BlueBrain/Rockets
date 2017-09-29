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

#ifndef ROCKETS_SERVICETHREADPOOL_H
#define ROCKETS_SERVICETHREADPOOL_H

#include <atomic>
#include <thread>
#include <vector>

#include <rockets/serverContext.h>

namespace rockets
{
/**
 * Service thread pool for the server.
 */
class ServiceThreadPool
{
public:
    ServiceThreadPool(ServerContext& context);
    ~ServiceThreadPool();

    size_t getSize() const;
    void requestBroadcast();

private:
    ServerContext& context;
    std::vector<std::thread> serviceThreads;
    std::unique_ptr<std::atomic_bool[]> broadcastRequested;
    bool exitService = false;

    void handleBroadcastRequest(int tsi);

    void start();
    void stop();
};
}

#endif
