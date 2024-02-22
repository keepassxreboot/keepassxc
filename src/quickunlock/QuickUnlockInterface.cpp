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

#include "QuickUnlockInterface.h"
#include <QObject>

#if defined(Q_OS_MACOS)
#include "TouchID.h"
#define QUICKUNLOCK_IMPLEMENTATION TouchID
#elif defined(Q_CC_MSVC)
#include "WindowsHello.h"
#define QUICKUNLOCK_IMPLEMENTATION WindowsHello
#elif defined(Q_OS_LINUX)
#include "Polkit.h"
#define QUICKUNLOCK_IMPLEMENTATION Polkit
#else
#define QUICKUNLOCK_IMPLEMENTATION NoQuickUnlock
#endif

QUICKUNLOCK_IMPLEMENTATION* quickUnlockInstance = {nullptr};

QuickUnlockInterface* getQuickUnlock()
{
    if (!quickUnlockInstance) {
        quickUnlockInstance = new QUICKUNLOCK_IMPLEMENTATION();
    }
    return quickUnlockInstance;
}

bool NoQuickUnlock::isAvailable() const
{
    return false;
}

QString NoQuickUnlock::errorString() const
{
    return QObject::tr("No Quick Unlock provider is available");
}

void NoQuickUnlock::reset()
{
}

bool NoQuickUnlock::setKey(const QUuid& dbUuid, const QByteArray& key)
{
    Q_UNUSED(dbUuid)
    Q_UNUSED(key)
    return false;
}

bool NoQuickUnlock::getKey(const QUuid& dbUuid, QByteArray& key)
{
    Q_UNUSED(dbUuid)
    Q_UNUSED(key)
    return false;
}

bool NoQuickUnlock::hasKey(const QUuid& dbUuid) const
{
    Q_UNUSED(dbUuid)
    return false;
}

void NoQuickUnlock::reset(const QUuid& dbUuid)
{
    Q_UNUSED(dbUuid)
}
