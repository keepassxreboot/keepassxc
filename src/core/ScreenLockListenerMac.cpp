#include "ScreenLockListenerMac.h"

#include <QMutexLocker>
#include <CoreFoundation/CoreFoundation.h>

ScreenLockListenerMac* ScreenLockListenerMac::instance(){
    static QMutex mutex;
    QMutexLocker lock(&mutex);

    static ScreenLockListenerMac* m_ptr=NULL;
    if (m_ptr==NULL){
        m_ptr = new ScreenLockListenerMac();
    }
    return m_ptr;
}

void ScreenLockListenerMac::notificationCenterCallBack(CFNotificationCenterRef /*center*/, void */*observer*/,
                                        CFNotificationName /*name*/, const void */*object*/, CFDictionaryRef /*userInfo*/){
    instance()->onSignalReception();
}

ScreenLockListenerMac::ScreenLockListenerMac(QWidget* parent):
    ScreenLockListenerPrivate(parent){
    CFNotificationCenterRef distCenter;
    CFStringRef screenIsLockedSignal = CFSTR("com.apple.screenIsLocked");
    distCenter = CFNotificationCenterGetDistributedCenter();
    if (NULL == distCenter)
        return;

    CFNotificationCenterAddObserver(
                distCenter,
                this, &ScreenLockListenerMac::notificationCenterCallBack,
                screenIsLockedSignal,
                NULL,
                CFNotificationSuspensionBehaviorDeliverImmediately);
}

void ScreenLockListenerMac::onSignalReception()
{
    Q_EMIT screenLocked();
}
