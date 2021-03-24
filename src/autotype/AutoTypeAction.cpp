/*
 *  Copyright (C) 2021 Team KeePassXC <team@keepassxc.org>
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

#include "AutoTypeAction.h"

#include "core/Tools.h"

AutoTypeKey::AutoTypeKey(Qt::Key key, Qt::KeyboardModifiers modifiers)
    : key(key)
    , modifiers(modifiers)
{
}

AutoTypeKey::AutoTypeKey(const QChar& character, Qt::KeyboardModifiers modifiers)
    : character(character)
    , modifiers(modifiers)
{
}

AutoTypeAction::Result AutoTypeKey::exec(AutoTypeExecutor* executor) const
{
    return executor->execType(this);
}

AutoTypeAction::Result AutoTypeKey::exec(TargetedAutoTypeExecutor* executor, const QSharedPointer<AutoTypeTarget>& target)
{
    return executor->execType(this, target);
}

AutoTypeDelay::AutoTypeDelay(int delayMs, bool setExecDelay)
    : delayMs(delayMs)
    , setExecDelay(setExecDelay)
{
}

AutoTypeAction::Result AutoTypeDelay::exec(AutoTypeExecutor* executor) const
{
    if (setExecDelay) {
        // Change the delay between actions
        executor->execDelayMs = delayMs;
    } else {
        // Pause execution
        Tools::wait(delayMs);
    }

    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeDelay::exec(TargetedAutoTypeExecutor* executor,
                                           const QSharedPointer<AutoTypeTarget>& target)
{
    Q_UNUSED(target);
    if (setExecDelay) {
        // Change the delay between actions
        executor->execDelayMs = delayMs;
    } else {
        // Pause execution
        Tools::wait(delayMs);
    }

    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeClearField::exec(AutoTypeExecutor* executor) const
{
    return executor->execClearField(this);
}

AutoTypeAction::Result AutoTypeClearField::exec(TargetedAutoTypeExecutor* executor,
                                                const QSharedPointer<AutoTypeTarget>& target)
{
    return executor->execClearField(this, target);
}

AutoTypeAction::Result AutoTypeBegin::exec(AutoTypeExecutor* executor) const
{
    return executor->execBegin(this);
}

AutoTypeAction::Result AutoTypeBegin::exec(TargetedAutoTypeExecutor* executor,
                                           const QSharedPointer<AutoTypeTarget>& target)
{
    return executor->execBegin(this, target);
}
