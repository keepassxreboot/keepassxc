/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "MacUtils.h"
#include <QApplication>

QPointer<MacUtils> MacUtils::m_instance = nullptr;

MacUtils::MacUtils(QObject* parent)
    : OSUtilsBase(parent)
    , m_appkit(new AppKit())
{
    connect(m_appkit.data(), SIGNAL(lockDatabases()), SIGNAL(lockDatabases()));
}

MacUtils::~MacUtils()
{
}

MacUtils* MacUtils::instance()
{
    if (!m_instance) {
        m_instance = new MacUtils(qApp);
    }

    return m_instance;
}

WId MacUtils::activeWindow()
{
    return m_appkit->activeProcessId();
}

bool MacUtils::raiseWindow(WId pid)
{
    return m_appkit->activateProcess(pid);
}

bool MacUtils::raiseOwnWindow()
{
    return m_appkit->activateProcess(m_appkit->ownProcessId());
}

bool MacUtils::raiseLastActiveWindow()
{
    return m_appkit->activateProcess(m_appkit->lastActiveProcessId());
}

bool MacUtils::hideOwnWindow()
{
    return m_appkit->hideProcess(m_appkit->ownProcessId());
}

bool MacUtils::isHidden()
{
    return m_appkit->isHidden(m_appkit->ownProcessId());
}

bool MacUtils::isDarkMode()
{
    return m_appkit->isDarkMode();
}

bool MacUtils::enableAccessibility()
{
    return m_appkit->enableAccessibility();
}

bool MacUtils::enableScreenRecording()
{
    return m_appkit->enableScreenRecording();
}
