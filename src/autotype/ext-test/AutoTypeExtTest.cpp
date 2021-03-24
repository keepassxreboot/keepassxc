/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#include "AutoTypeExtTest.h"

QString AutoTypeExtTest::keyToString(Qt::Key key)
{
    return QString("[Key0x%1]").arg(key, 0, 16);
}

bool AutoTypeExtTest::isAvailable()
{
    return true;
}

TargetedAutoTypeExecutor* AutoTypeExtTest::createExecutor()
{
    return new AutoTypeExtExecutorTest(this);
}

void AutoTypeExtTest::setTargetMap(const AutoTypeTargetMap& targetMap)
{
    m_targetMap = targetMap;
}

AutoTypeTargetMap AutoTypeExtTest::availableTargets()
{
    return m_targetMap;
}

QString AutoTypeExtTest::actionCharsForTarget(const QString& targetIdentifier)
{
    if (m_actionCharsPerTarget.contains(targetIdentifier)) {
        return m_actionCharsPerTarget[targetIdentifier];
    }
    return "";
}

int AutoTypeExtTest::actionCountForTarget(const QString& targetIdentifier)
{
    if (m_actionCountPerTarget.contains(targetIdentifier)) {
        return m_actionCountPerTarget[targetIdentifier];
    }
    return 0;
}

void AutoTypeExtTest::clearActions()
{
    m_actionCharsPerTarget.clear();
    m_actionCountPerTarget.clear();
}

void AutoTypeExtTest::addAction(const QSharedPointer<AutoTypeTarget>& target, const AutoTypeKey* action)
{
    if (!m_actionCountPerTarget.contains(target->getIdentifier())) {
        m_actionCountPerTarget[target->getIdentifier()] = 0;
        m_actionCharsPerTarget[target->getIdentifier()] = "";
    }

    m_actionCountPerTarget[target->getIdentifier()]++;

    if (action->key != Qt::Key_unknown) {
        m_actionCharsPerTarget[target->getIdentifier()] += keyToString(action->key);
    } else {
        m_actionCharsPerTarget[target->getIdentifier()] += action->character;
    }
}

AutoTypeExtExecutorTest::AutoTypeExtExecutorTest(AutoTypeExtTest* platform)
    : m_platform(platform)
{
}

void AutoTypeExtExecutorTest::execType(const QSharedPointer<AutoTypeTarget>& target, AutoTypeKey* action)
{
    m_platform->addAction(target, action);
}

void AutoTypeExtExecutorTest::execClearField(const QSharedPointer<AutoTypeTarget>& target, AutoTypeClearField* action)
{
    Q_UNUSED(target);
    Q_UNUSED(action);
}
