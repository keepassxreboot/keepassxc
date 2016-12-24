#ifndef SCREENLOCKLISTENERPRIVATE_H
#define SCREENLOCKLISTENERPRIVATE_H
#include <QObject>
#include <QWidget>

class ScreenLockListenerPrivate : public QObject
{
    Q_OBJECT
public:
    static ScreenLockListenerPrivate* instance(QWidget *parent = 0);

protected:
    ScreenLockListenerPrivate(QWidget *parent = 0);

Q_SIGNALS:
    void screenLocked();
};

#endif // SCREENLOCKLISTENERPRIVATE_H
