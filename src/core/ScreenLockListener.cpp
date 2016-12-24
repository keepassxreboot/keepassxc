#include "ScreenLockListener.h"
#include "ScreenLockListenerPrivate.h"

ScreenLockListener::ScreenLockListener(QWidget* parent):
    QObject(parent){
    m_listener = ScreenLockListenerPrivate::instance(parent);
    connect(m_listener,SIGNAL(screenLocked()), this,SIGNAL(screenLocked()));
}

ScreenLockListener::~ScreenLockListener(){
}
