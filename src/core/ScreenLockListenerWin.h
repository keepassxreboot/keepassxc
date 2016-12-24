#ifndef SCREENLOCKLISTENERWIN_H
#define SCREENLOCKLISTENERWIN_H
#include <QObject>
#include <QWidget>
#include <QAbstractNativeEventFilter>

#include "ScreenLockListenerPrivate.h"

class ScreenLockListenerWin : public ScreenLockListenerPrivate, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit ScreenLockListenerWin(QWidget *parent = 0);
    ~ScreenLockListenerWin();
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *) Q_DECL_OVERRIDE;

private:
    void * m_powernotificationhandle;
};

#endif // SCREENLOCKLISTENERWIN_H
