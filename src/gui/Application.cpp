/*
 *  Copyright (C) 2012 Tobias Tangemann
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "Application.h"
#include "MainWindow.h"

#include <QAbstractNativeEventFilter>
#include <QFileOpenEvent>
#include <QSocketNotifier>

#include "autotype/AutoType.h"

#if defined(Q_OS_UNIX)
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_OSX)
class XcbEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override
    {
        Q_UNUSED(result)

        if (eventType == QByteArrayLiteral("xcb_generic_event_t")) {
            int retCode = autoType()->callEventFilter(message);
            if (retCode == 1) {
                return true;
            }
        }

        return false;
    }
};
#elif defined(Q_OS_WIN)
class WinEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override
    {
        Q_UNUSED(result);

        if (eventType == QByteArrayLiteral("windows_generic_MSG")
                || eventType == QByteArrayLiteral("windows_dispatcher_MSG")) {
            int retCode = autoType()->callEventFilter(message);
            if (retCode == 1) {
                return true;
            }
        }

        return false;
    }
};
#endif

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
    , m_mainWindow(nullptr)
#ifdef Q_OS_UNIX
    , m_unixSignalNotifier(nullptr)
#endif
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_OSX)
    installNativeEventFilter(new XcbEventFilter());
#elif defined(Q_OS_WIN)
    installNativeEventFilter(new WinEventFilter());
#endif
#if defined(Q_OS_UNIX)
    registerUnixSignals();
#endif
}

void Application::setMainWindow(QWidget* mainWindow)
{
    m_mainWindow = mainWindow;
}

bool Application::event(QEvent* event)
{
    // Handle Apple QFileOpenEvent from finder (double click on .kdbx file)
    if (event->type() == QEvent::FileOpen) {
        Q_EMIT openFile(static_cast<QFileOpenEvent*>(event)->file());
        return true;
    }
#ifdef Q_OS_MAC
    // restore main window when clicking on the docker icon
    else if ((event->type() == QEvent::ApplicationActivate) && m_mainWindow) {
        m_mainWindow->ensurePolished();
        m_mainWindow->setWindowState(m_mainWindow->windowState() & ~Qt::WindowMinimized);
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    }
#endif

    return QApplication::event(event);
}

#if defined(Q_OS_UNIX)
int Application::unixSignalSocket[2];

void Application::registerUnixSignals()
{
    int result = ::socketpair(AF_UNIX, SOCK_STREAM, 0, unixSignalSocket);
    Q_ASSERT(0 == result);
    if (0 != result) {
        // do not register handles when socket creation failed, otherwise
        // application will be unresponsive to signals such as SIGINT or SIGTERM
        return;
    }
    
    QVector<int> const handledSignals = { SIGQUIT, SIGINT, SIGTERM, SIGHUP };
    for (auto s: handledSignals) {
        struct sigaction sigAction;
        
        sigAction.sa_handler = handleUnixSignal;
        sigemptyset(&sigAction.sa_mask);
        sigAction.sa_flags = 0 | SA_RESTART;
        sigaction(s, &sigAction, nullptr);
    }
    
    m_unixSignalNotifier = new QSocketNotifier(unixSignalSocket[1], QSocketNotifier::Read, this);
    connect(m_unixSignalNotifier, SIGNAL(activated(int)), this, SLOT(quitBySignal()));
}

void Application::handleUnixSignal(int sig)
{
    switch (sig) {
        case SIGQUIT:
        case SIGINT:
        case SIGTERM:
        {
            char buf = 0;
            ::write(unixSignalSocket[0], &buf, sizeof(buf));
            return;
        }
        case SIGHUP:
            return;
    }
}

void Application::quitBySignal()
{
    m_unixSignalNotifier->setEnabled(false);
    char buf;
    ::read(unixSignalSocket[1], &buf, sizeof(buf));
    
    if (nullptr != m_mainWindow)
        static_cast<MainWindow*>(m_mainWindow)->appExit();
}
#endif
