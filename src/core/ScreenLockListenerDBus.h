#ifndef SCREENLOCKLISTENERDBUS_H
#define SCREENLOCKLISTENERDBUS_H
#include <QObject>
#include <QWidget>
#include "ScreenLockListenerPrivate.h"

class ScreenLockListenerDBus : public ScreenLockListenerPrivate
{
    Q_OBJECT
public:
    explicit ScreenLockListenerDBus(QWidget *parent = 0);

private Q_SLOTS:
    void gnomeSessionStatusChanged(uint status);
    void logindPrepareForSleep(bool beforeSleep);
    void unityLocked();
    void freedesktopScreenSaver(bool status);
};

#endif // SCREENLOCKLISTENERDBUS_H
