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

#ifndef ROCKETS_QT_READWRITESOCKETNOTIFIER_H
#define ROCKETS_QT_READWRITESOCKETNOTIFIER_H

#include <rockets/types.h>

#include <QSocketNotifier>

#include <functional>
#include <poll.h>

namespace rockets
{
namespace qt
{
/**
 * Notifier to monitor read+write activity on a socket as part of a QEventLoop.
 */
class ReadWriteSocketNotifier
{
public:
    ReadWriteSocketNotifier(const SocketDescriptor fd)
        : read{new QSocketNotifier{fd, QSocketNotifier::Read}}
        , write{new QSocketNotifier{fd, QSocketNotifier::Write}}
    {
    }

    ~ReadWriteSocketNotifier()
    {
        read->setEnabled(false);
        read->deleteLater();
        write->setEnabled(false);
        write->deleteLater();
    }

    void enable(const int mode)
    {
        read->setEnabled(mode & POLLIN);
        write->setEnabled(mode & POLLOUT);
    }

    void onReadReady(std::function<void()> callback)
    {
        QObject::connect(read, &QSocketNotifier::activated, callback);
    }

    void onWriteReady(std::function<void()> callback)
    {
        QObject::connect(write, &QSocketNotifier::activated, callback);
    }

private:
    QSocketNotifier* read = nullptr;
    QSocketNotifier* write = nullptr;
};
}
}

#endif
