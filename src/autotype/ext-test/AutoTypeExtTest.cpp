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

bool AutoTypeExtTest::isTargetSelectionRequired()
{
    return m_isTargetSelectionRequired;
}

TargetedAutoTypeExecutor* AutoTypeExtTest::createExecutor()
{
    return new AutoTypeExtExecutorTest(this);
}

AutoTypeTargetMap AutoTypeExtTest::availableTargets()
{
    return m_targetMap;
}

QString AutoTypeExtTest::getActionChars()
{
    return m_actionCharsTargetLess;
}

QString AutoTypeExtTest::getActionChars(const QString& targetIdentifier)
{
    if (m_actionCharsPerTarget.contains(targetIdentifier)) {
        return m_actionCharsPerTarget[targetIdentifier];
    }
    return "";
}

int AutoTypeExtTest::getActionCount()
{
    return m_actionCountTargetless;
}

int AutoTypeExtTest::getActionCount(const QString& targetIdentifier)
{
    if (m_actionCountPerTarget.contains(targetIdentifier)) {
        return m_actionCountPerTarget[targetIdentifier];
    }
    return 0;
}

void AutoTypeExtTest::setTargetSelectionRequired(bool value)
{
    m_isTargetSelectionRequired = value;
}

void AutoTypeExtTest::setTargetMap(const AutoTypeTargetMap& targetMap)
{
    m_targetMap = targetMap;
}

void AutoTypeExtTest::clearActions()
{
    m_actionCharsPerTarget.clear();
    m_actionCountPerTarget.clear();
    m_actionCharsTargetLess.clear();
    m_actionCountTargetless = 0;
}

void AutoTypeExtTest::addAction(QSharedPointer<AutoTypeTarget> target, const AutoTypeKey* action)
{
    QString toAppend = (action->key != Qt::Key_unknown) ? keyToString(action->key) : action->character;

    if (!m_isTargetSelectionRequired) {
        m_actionCountTargetless++;
        m_actionCharsTargetLess += toAppend;
        return;
    }

    if (!m_actionCountPerTarget.contains(target->getIdentifier())) {
        m_actionCountPerTarget[target->getIdentifier()] = 0;
        m_actionCharsPerTarget[target->getIdentifier()] = "";
    }

    m_actionCountPerTarget[target->getIdentifier()]++;
    m_actionCharsPerTarget[target->getIdentifier()] += toAppend;
}

AutoTypeExtExecutorTest::AutoTypeExtExecutorTest(AutoTypeExtTest* platform)
    : m_platform(platform)
{
}

AutoTypeAction::Result AutoTypeExtExecutorTest::execBegin(const AutoTypeBegin* action,
                                                          QSharedPointer<AutoTypeTarget> target)
{
    Q_UNUSED(target);
    Q_UNUSED(action);
    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExtExecutorTest::execType(AutoTypeKey* action, QSharedPointer<AutoTypeTarget> target)
{
    m_platform->addAction(target, action);
    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExtExecutorTest::execClearField(AutoTypeClearField* action,
                                                               QSharedPointer<AutoTypeTarget> target)
{
    Q_UNUSED(target);
    Q_UNUSED(action);
    return AutoTypeAction::Result::Ok();
}
