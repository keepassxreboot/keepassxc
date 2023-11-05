/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "RemoteProcessFactory.h"

#include <utility>

std::function<QScopedPointer<RemoteProcess>(QObject*)> RemoteProcessFactory::m_createRemoteProcess([](QObject* parent) {
    return QScopedPointer<RemoteProcess>(new RemoteProcess(parent));
});

QScopedPointer<RemoteProcess> RemoteProcessFactory::createRemoteProcess(QObject* parent)
{
    return m_createRemoteProcess(parent);
}

// use for testing only
void RemoteProcessFactory::setCreateRemoteProcessFunc(
    std::function<QScopedPointer<RemoteProcess>(QObject*)> createRemoteProcessFunc)
{
    m_createRemoteProcess = std::move(createRemoteProcessFunc);
}
