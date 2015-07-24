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

class AutoTypePlatformTest : public QObject,
                             public AutoTypePlatformInterface,
                             public AutoTypeTestInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.keepassx.AutoTypePlatformInterface")
    Q_INTERFACES(AutoTypePlatformInterface AutoTypeTestInterface)

public:
    QString keyToString(Qt::Key key);

    QStringList windowTitles();
    WId activeWindow();
    QString activeWindowTitle();
    bool registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers);
    void unregisterGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers);
    int platformEventFilter(void* event);
    int initialTimeout();
    AutoTypeExecutor* createExecutor();

    void setActiveWindowTitle(const QString& title);

    QString actionChars();
    int actionCount();
    void clearActions();

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

    void execChar(AutoTypeChar* action);
    void execKey(AutoTypeKey* action);

private:
    AutoTypePlatformTest* const m_platform;
};

#endif // KEEPASSX_AUTOTYPETEST_H
