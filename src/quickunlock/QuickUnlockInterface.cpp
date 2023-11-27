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
#include "PinUnlock.h"

#include <QObject>

#if defined(Q_OS_MACOS)
#include "TouchID.h"
#elif defined(Q_CC_MSVC)
#include "WindowsHello.h"
#elif defined(Q_OS_LINUX)
#include "Polkit.h"
#endif

QuickUnlockManager* quickUnlockManager = nullptr;

QuickUnlockManager::QuickUnlockManager()
{
#if defined(Q_OS_MACOS)
    m_interfaces.append(new TouchId());
#elif defined(Q_CC_MSVC)
    m_interfaces.append(new WindowsHello());
#elif defined(Q_OS_LINUX)
    m_interfaces.append(new Polkit());
#endif
    m_interfaces.append(new PinUnlock());
}

const QuickUnlockManager* QuickUnlockManager::get()
{
    if (!quickUnlockManager) {
        quickUnlockManager = new QuickUnlockManager();
    }
    return quickUnlockManager;
}

QuickUnlockInterface* QuickUnlockManager::getQuickUnlock() const
{
    for (auto* interface : m_interfaces) {
        if (interface->isAvailable()) {
            return interface;
        }
    }
    return nullptr;
}
