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

#ifndef SCREENLOCKLISTENERMAC_H
#define SCREENLOCKLISTENERMAC_H
#include <QObject>
#include <QWidget>

#include <CoreFoundation/CoreFoundation.h>

#include "ScreenLockListenerPrivate.h"

class ScreenLockListenerMac : public ScreenLockListenerPrivate
{
    Q_OBJECT

public:
    static ScreenLockListenerMac* instance();
    static void notificationCenterCallBack(CFNotificationCenterRef center,
                                           void* observer,
                                           CFStringRef name,
                                           const void* object,
                                           CFDictionaryRef userInfo);

private:
    ScreenLockListenerMac(QWidget* parent = nullptr);
    void onSignalReception();
};

#endif // SCREENLOCKLISTENERMAC_H
