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

#include <QAbstractNativeEventFilter>
#include <QFileOpenEvent>

#include "autotype/AutoType.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_OSX)
class XcbEventFilter : public QAbstractNativeEventFilter
{
public:
    virtual bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) Q_DECL_OVERRIDE
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
#endif

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_OSX)
    installNativeEventFilter(new XcbEventFilter());
#endif
}

bool Application::event(QEvent* event)
{
    // Handle Apple QFileOpenEvent from finder (double click on .kdbx file)
    if (event->type() == QEvent::FileOpen) {
        Q_EMIT openFile(static_cast<QFileOpenEvent*>(event)->file());
        return true;
    }

    return QApplication::event(event);
}
