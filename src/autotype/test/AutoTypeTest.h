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

#include "autotype/AutoTypeAction.h"
#include "autotype/AutoTypePlatformPlugin.h"
#include "autotype/test/AutoTypeTestInterface.h"

class AutoTypePlatformTest : public QObject, public AutoTypePlatformInterface, public AutoTypeTestInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.keepassx.AutoTypePlatformInterface")
    Q_INTERFACES(AutoTypePlatformInterface AutoTypeTestInterface)

public:
    QString keyToString(Qt::Key key) override;

    bool isAvailable() override;
    QStringList windowTitles() override;
    WId activeWindow() override;
    QString activeWindowTitle() override;
    bool raiseWindow(WId window) override;
    AutoTypeExecutor* createExecutor() override;

#if defined(Q_OS_MACOS)
    bool hideOwnWindow() override;
    bool raiseOwnWindow() override;
#endif

    void setActiveWindowTitle(const QString& title) override;

    QString actionChars() override;
    int actionCount() override;
    void clearActions() override;

    void addActionChar(AutoTypeChar* action);
    void addActionKey(AutoTypeKey* action);

private:
    QString m_activeWindowTitle;
    QList<AutoTypeAction*> m_actionList;
    QString m_actionChars;
};

class AutoTypeExecutorTest : public AutoTypeExecutor
{
public:
    explicit AutoTypeExecutorTest(AutoTypePlatformTest* platform);

    void execChar(AutoTypeChar* action) override;
    void execKey(AutoTypeKey* action) override;

private:
    AutoTypePlatformTest* const m_platform;
};

#endif // KEEPASSX_AUTOTYPETEST_H
