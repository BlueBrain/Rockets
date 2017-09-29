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

#ifndef ROCKETS_QT_SOCKETPROCESSOR_H
#define ROCKETS_QT_SOCKETPROCESSOR_H

#include <rockets/qt/readWriteSocketNotifier.h>
#include <rockets/socketBasedInterface.h>
#include <rockets/socketListener.h>

#include <map>

namespace rockets
{
namespace qt
{
/**
 * Helper class to process any SocketBasedInterface as part of a QEventLoop.
 */
class SocketProcessor : public SocketListener
{
public:
    SocketProcessor(SocketBasedInterface& interface)
        : iface{interface}
    {
    }

    void onNewSocket(const SocketDescriptor fd, const int mode) final
    {
        notifiers.emplace(fd, fd);
        auto& notifier = notifiers.at(fd);
        notifier.enable(mode);
        notifier.onReadReady([this, fd] { iface.processSocket(fd, POLLIN); });
        notifier.onWriteReady([this, fd] { iface.processSocket(fd, POLLOUT); });
    }

    void onUpdateSocket(const SocketDescriptor fd, const int mode) final
    {
        notifiers.at(fd).enable(mode);
    }

    void onDeleteSocket(const SocketDescriptor fd) final
    {
        notifiers.erase(fd);
    }

private:
    SocketBasedInterface& iface;
    std::map<SocketDescriptor, ReadWriteSocketNotifier> notifiers;
};
}
}

#endif
