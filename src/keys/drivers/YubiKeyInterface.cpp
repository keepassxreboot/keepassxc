/*
 *  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
 *  Copyright (C) 2017-2021 KeePassXC Team <team@keepassxc.org>
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

#include "YubiKeyInterface.h"

YubiKeyInterface::YubiKeyInterface()
    : m_mutex(QMutex::Recursive)
{
    m_interactionTimer.setSingleShot(true);
    m_interactionTimer.setInterval(300);
}

bool YubiKeyInterface::isInitialized() const
{
    return m_initialized;
}

QMultiHash<unsigned int, YubiKeyInterface::KeyData> YubiKeyInterface::foundKeys()
{
    return m_foundKeys;
}

bool YubiKeyInterface::hasFoundKey(YubiKeySlot slot)
{
    for (const auto& key : m_foundKeys.values(slot.first)) {
        if (slot.second == key.slot) {
            return true;
        }
    }
    return false;
}

QString YubiKeyInterface::getDisplayName(YubiKeySlot slot)
{
    for (const auto& key : m_foundKeys.values(slot.first)) {
        if (slot.second == key.slot) {
            return key.desc;
        }
    }
    return tr("%1 Invalid slot specified - %2").arg(QString::number(slot.first), QString::number(slot.second));
}

QString YubiKeyInterface::errorMessage()
{
    return m_error;
}
