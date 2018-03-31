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

#include "AutoTypeAction.h"

#include "core/Tools.h"

AutoTypeChar::AutoTypeChar(const QChar& character)
    : character(character)
{
}

AutoTypeAction* AutoTypeChar::clone()
{
    return new AutoTypeChar(character);
}

void AutoTypeChar::accept(AutoTypeExecutor* executor)
{
    executor->execChar(this);
}

AutoTypeKey::AutoTypeKey(Qt::Key key)
    : key(key)
{
}

AutoTypeAction* AutoTypeKey::clone()
{
    return new AutoTypeKey(key);
}

void AutoTypeKey::accept(AutoTypeExecutor* executor)
{
    executor->execKey(this);
}

AutoTypeDelay::AutoTypeDelay(int delayMs)
    : delayMs(delayMs)
{
}

AutoTypeAction* AutoTypeDelay::clone()
{
    return new AutoTypeDelay(delayMs);
}

void AutoTypeDelay::accept(AutoTypeExecutor* executor)
{
    executor->execDelay(this);
}

AutoTypeClearField::AutoTypeClearField()
{
}

AutoTypeAction* AutoTypeClearField::clone()
{
    return new AutoTypeClearField();
}

void AutoTypeClearField::accept(AutoTypeExecutor* executor)
{
    executor->execClearField(this);
}

void AutoTypeExecutor::execDelay(AutoTypeDelay* action)
{
    Tools::wait(action->delayMs);
}

void AutoTypeExecutor::execClearField(AutoTypeClearField* action)
{
    Q_UNUSED(action);
}
