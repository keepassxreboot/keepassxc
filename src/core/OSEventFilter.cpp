/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OSEventFilter.h"

#include <QByteArray>

#include "autotype/AutoType.h"
#include "gui/MainWindow.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

OSEventFilter::OSEventFilter()
{
}

bool OSEventFilter::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
{
    Q_UNUSED(result)

#if defined(Q_OS_UNIX)
    if (eventType == QByteArrayLiteral("xcb_generic_event_t")) {
#elif defined(Q_OS_WIN)
    if (eventType == QByteArrayLiteral("windows_generic_MSG")
        || eventType == QByteArrayLiteral("windows_dispatcher_MSG")) {
#endif
        return autoType()->callEventFilter(message) == 1;
    }

    return false;
}
