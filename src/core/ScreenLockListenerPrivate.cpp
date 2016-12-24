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

ScreenLockListenerPrivate::ScreenLockListenerPrivate(QWidget* parent):
    QObject(parent){
}

ScreenLockListenerPrivate* ScreenLockListenerPrivate::instance(QWidget* parent){
#if defined(Q_OS_OSX)
    return ScreenLockListenerMac::instance();
#endif
#if defined(Q_OS_LINUX)
    return new ScreenLockListenerDBus(parent);
#endif
#if defined(Q_OS_WIN)
    return new ScreenLockListenerWin(parent);
#endif
}
