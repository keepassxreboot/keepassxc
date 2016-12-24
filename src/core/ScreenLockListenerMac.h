#ifndef SCREENLOCKLISTENERMAC_H
#define SCREENLOCKLISTENERMAC_H
#include <QObject>
#include <QWidget>

#include <CoreFoundation/CoreFoundation.h>

#include "ScreenLockListenerPrivate.h"

class ScreenLockListenerMac: public ScreenLockListenerPrivate {
    Q_OBJECT

public:
    static ScreenLockListenerMac* instance();
    static void notificationCenterCallBack(CFNotificationCenterRef /*center*/, void */*observer*/,
                            CFNotificationName /*name*/, const void */*object*/, CFDictionaryRef /*userInfo*/);

private:
    ScreenLockListenerMac(QWidget* parent=NULL);
    void onSignalReception();

};

#endif // SCREENLOCKLISTENERMAC_H
