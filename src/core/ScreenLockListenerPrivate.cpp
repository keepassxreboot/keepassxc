/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "ScreenLockListenerPrivate.h"
#if defined(Q_OS_OSX)
#include "ScreenLockListenerMac.h"
#endif
#if defined(Q_OS_LINUX)
#include "ScreenLockListenerDBus.h"
#endif
#if defined(Q_OS_WIN)
#include "ScreenLockListenerWin.h"
#endif

ScreenLockListenerPrivate::ScreenLockListenerPrivate(QWidget* parent)
	: QObject(parent)
{
}

ScreenLockListenerPrivate* ScreenLockListenerPrivate::instance(QWidget* parent)
{
#if defined(Q_OS_OSX)
    Q_UNUSED(parent);
    return ScreenLockListenerMac::instance();
#elif defined(Q_OS_LINUX)
    return new ScreenLockListenerDBus(parent);
#elif defined(Q_OS_WIN)
    return new ScreenLockListenerWin(parent);
#endif
}
