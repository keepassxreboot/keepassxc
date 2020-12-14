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

#include "AutoTypeTest.h"

bool AutoTypePlatformTest::isAvailable()
{
    return true;
}

QString AutoTypePlatformTest::keyToString(Qt::Key key)
{
    return QString("[Key0x%1]").arg(key, 0, 16);
}

QStringList AutoTypePlatformTest::windowTitles()
{
    return QStringList();
}

WId AutoTypePlatformTest::activeWindow()
{
    return 0;
}

QString AutoTypePlatformTest::activeWindowTitle()
{
    return m_activeWindowTitle;
}

AutoTypeExecutor* AutoTypePlatformTest::createExecutor()
{
    return new AutoTypeExecutorTest(this);
}

void AutoTypePlatformTest::setActiveWindowTitle(const QString& title)
{
    m_activeWindowTitle = title;
}

QString AutoTypePlatformTest::actionChars()
{
    return m_actionChars;
}

int AutoTypePlatformTest::actionCount()
{
    return m_actionList.size();
}

void AutoTypePlatformTest::clearActions()
{
    qDeleteAll(m_actionList);
    m_actionList.clear();

    m_actionChars.clear();
}

void AutoTypePlatformTest::addActionChar(AutoTypeChar* action)
{
    m_actionList.append(action->clone());
    m_actionChars += action->character;
}

void AutoTypePlatformTest::addActionKey(AutoTypeKey* action)
{
    m_actionList.append(action->clone());
    m_actionChars.append(keyToString(action->key));
}

bool AutoTypePlatformTest::raiseWindow(WId window)
{
    Q_UNUSED(window);

    return false;
}

#if defined(Q_OS_MACOS)
bool AutoTypePlatformTest::hideOwnWindow()
{
    return false;
}

bool AutoTypePlatformTest::raiseOwnWindow()
{
    return false;
}
#endif

AutoTypeExecutorTest::AutoTypeExecutorTest(AutoTypePlatformTest* platform)
    : m_platform(platform)
{
}

void AutoTypeExecutorTest::execChar(AutoTypeChar* action)
{
    m_platform->addActionChar(action);
}

void AutoTypeExecutorTest::execKey(AutoTypeKey* action)
{
    m_platform->addActionKey(action);
}
