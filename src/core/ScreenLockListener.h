#ifndef SCREENLOCKLISTENER_H
#define SCREENLOCKLISTENER_H
#include <QWidget>

class ScreenLockListenerPrivate;

class ScreenLockListener : public QObject {
    Q_OBJECT

public:
    ScreenLockListener(QWidget* parent=NULL);
    ~ScreenLockListener();

Q_SIGNALS:
    void screenLocked();

private:
    ScreenLockListenerPrivate* m_listener;
};

#endif // SCREENLOCKLISTENER_H
