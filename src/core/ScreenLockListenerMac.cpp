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

#include "ScreenLockListenerMac.h"

#include <CoreFoundation/CoreFoundation.h>
#include <QMutexLocker>

ScreenLockListenerMac* ScreenLockListenerMac::instance()
{
    static QMutex mutex;
    QMutexLocker lock(&mutex);

    static ScreenLockListenerMac* m_ptr = nullptr;
    if (m_ptr == nullptr) {
        m_ptr = new ScreenLockListenerMac();
    }
    return m_ptr;
}

void ScreenLockListenerMac::notificationCenterCallBack(CFNotificationCenterRef,
                                                       void*,
                                                       CFStringRef,
                                                       const void*,
                                                       CFDictionaryRef)
{
    instance()->onSignalReception();
}

ScreenLockListenerMac::ScreenLockListenerMac(QWidget* parent)
    : ScreenLockListenerPrivate(parent)
{
    CFNotificationCenterRef distCenter;
    CFStringRef screenIsLockedSignal = CFSTR("com.apple.screenIsLocked");
    distCenter = CFNotificationCenterGetDistributedCenter();
    if (nullptr == distCenter) {
        return;
    }

    CFNotificationCenterAddObserver(distCenter,
                                    this,
                                    &ScreenLockListenerMac::notificationCenterCallBack,
                                    screenIsLockedSignal,
                                    nullptr,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);
}

void ScreenLockListenerMac::onSignalReception()
{
    emit screenLocked();
}
