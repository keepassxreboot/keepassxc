#include "ScreenLockListenerWin.h"
#include <QApplication>
#include <windows.h>
#include <wtsapi32.h>

/*
 * See https://msdn.microsoft.com/en-us/library/windows/desktop/aa373196(v=vs.85).aspx
 * See https://msdn.microsoft.com/en-us/library/aa383841(v=vs.85).aspx
 * See https://blogs.msdn.microsoft.com/oldnewthing/20060104-50/?p=32783
 */
ScreenLockListenerWin::ScreenLockListenerWin(QWidget *parent) :
    ScreenLockListenerPrivate(parent),
    QAbstractNativeEventFilter()
{
    Q_ASSERT(parent!=NULL);
    // On windows, we need to register for platform specific messages and
    // install a message handler for them
    QCoreApplication::instance()->installNativeEventFilter(this);

    // This call requests a notification from windows when a laptop is closed
    HPOWERNOTIFY hPnotify = RegisterPowerSettingNotification(
                reinterpret_cast<HWND>(parent->winId()),
                &GUID_LIDSWITCH_STATE_CHANGE, DEVICE_NOTIFY_WINDOW_HANDLE);
    m_powernotificationhandle = reinterpret_cast<void*>(hPnotify);

    // This call requests a notification for session changes
    if (!WTSRegisterSessionNotification(
                reinterpret_cast<HWND>(parent->winId()),
                NOTIFY_FOR_THIS_SESSION)){
    }
}

ScreenLockListenerWin::~ScreenLockListenerWin()
{
    HWND h= reinterpret_cast<HWND>(static_cast<QWidget*>(parent())->winId());
    WTSUnRegisterSessionNotification(h);

    if(m_powernotificationhandle){
        UnregisterPowerSettingNotification(reinterpret_cast<HPOWERNOTIFY>(m_powernotificationhandle));
    }
}

bool ScreenLockListenerWin::nativeEventFilter(const QByteArray &eventType, void *message, long *)
{
    if(eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG"){
        MSG* m = static_cast<MSG *>(message);
        if(m->message == WM_POWERBROADCAST){
            const POWERBROADCAST_SETTING* setting = reinterpret_cast<const POWERBROADCAST_SETTING*>(m->lParam);
            if (setting->PowerSetting == GUID_LIDSWITCH_STATE_CHANGE){
                const DWORD* state = reinterpret_cast<const DWORD*>(&setting->Data);
                if (*state == 0){
                    Q_EMIT screenLocked();
                    return true;
                }
            }
        }
        if(m->message == WM_WTSSESSION_CHANGE){
            if (m->wParam==WTS_CONSOLE_DISCONNECT){
                Q_EMIT screenLocked();
                return true;
            }
            if (m->wParam==WTS_SESSION_LOCK){
                Q_EMIT screenLocked();
                return true;
            }
        }
    }
    return false;
}
