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

#ifndef KEEPASSX_AUTOTYPETEST_H
#define KEEPASSX_AUTOTYPETEST_H

#include <QtPlugin>

#include "autotype/AutoTypePlatformPlugin.h"
#include "autotype/AutoTypeAction.h"
#include "autotype/test/AutoTypeTestInterface.h"
#include "core/Global.h"

class AutoTypePlatformTest : public QObject,
                             public AutoTypePlatformInterface,
                             public AutoTypeTestInterface
{
    Q_OBJECT
    Q_INTERFACES(AutoTypePlatformInterface AutoTypeTestInterface)

public:
    QString keyToString(Qt::Key key) Q_DECL_OVERRIDE;

    bool isAvailable() Q_DECL_OVERRIDE;
    QStringList windowTitles() Q_DECL_OVERRIDE;
    WId activeWindow() Q_DECL_OVERRIDE;
    QString activeWindowTitle() Q_DECL_OVERRIDE;
    bool registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers) Q_DECL_OVERRIDE;
    void unregisterGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers) Q_DECL_OVERRIDE;
    int platformEventFilter(void* event) Q_DECL_OVERRIDE;
    int initialTimeout() Q_DECL_OVERRIDE;
    bool raiseWindow(WId window) Q_DECL_OVERRIDE;
    AutoTypeExecutor* createExecutor() Q_DECL_OVERRIDE;

    void setActiveWindowTitle(const QString& title) Q_DECL_OVERRIDE;

    QString actionChars() Q_DECL_OVERRIDE;
    int actionCount() Q_DECL_OVERRIDE;
    void clearActions() Q_DECL_OVERRIDE;

    void addActionChar(AutoTypeChar* action);
    void addActionKey(AutoTypeKey* action);

Q_SIGNALS:
    void globalShortcutTriggered();

private:
    QString m_activeWindowTitle;
    QList<AutoTypeAction*> m_actionList;
    QString m_actionChars;
};

class AutoTypeExecturorTest : public AutoTypeExecutor
{
public:
    explicit AutoTypeExecturorTest(AutoTypePlatformTest* platform);

    void execChar(AutoTypeChar* action) Q_DECL_OVERRIDE;
    void execKey(AutoTypeKey* action) Q_DECL_OVERRIDE;

private:
    AutoTypePlatformTest* const m_platform;
};

#endif // KEEPASSX_AUTOTYPETEST_H
