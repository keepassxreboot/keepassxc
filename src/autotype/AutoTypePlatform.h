/*
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

#ifndef KEEPASSXC_AUTOTYPEPLATFORM_H
#define KEEPASSXC_AUTOTYPEPLATFORM_H

#include <QWidget>

#include "autotype/AutoTypeAction.h"

class AutoTypePlatformInterface
{
public:
    virtual ~AutoTypePlatformInterface() = default;

    virtual bool isAvailable() = 0;
    virtual QStringList windowTitles() = 0;
    virtual WId activeWindow() = 0;
    virtual QString activeWindowTitle() = 0;
    virtual bool raiseWindow(WId window) = 0;
    virtual bool hasWindowAccess()
    {
        return true;
    }

    virtual void prepareAutoType()
    {
    }
    virtual void finishAutoType()
    {
    }

#if defined(Q_OS_WIN32)
    // Called once the target window is known. Distinct from prepareAutoType(),
    // which carries no window, and raiseWindow(), which the entry-level path skips.
    virtual void beginSequence(WId window)
    {
        Q_UNUSED(window);
    }
    // Called when the sequence is over. May report a failure only the teardown can know.
    virtual AutoTypeAction::Result endSequence()
    {
        return AutoTypeAction::Result::Ok();
    }
#endif

    virtual AutoTypeExecutor& executor() const = 0;

#if defined(Q_OS_MACOS)
    virtual bool hideOwnWindow() = 0;
    virtual bool raiseOwnWindow() = 0;
#endif
};

#endif // KEEPASSXC_AUTOTYPEPLATFORM_H
