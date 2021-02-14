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

#ifndef KEEPASSX_AUTOTYPEACTION_H
#define KEEPASSX_AUTOTYPEACTION_H

#include <QChar>

#include "core/Global.h"

class AutoTypeExecutor;

class KEEPASSXC_EXPORT AutoTypeAction
{
public:
    AutoTypeAction() = default;
    virtual void exec(AutoTypeExecutor* executor) const = 0;
    virtual ~AutoTypeAction() = default;
};

class KEEPASSXC_EXPORT AutoTypeKey : public AutoTypeAction
{
public:
    explicit AutoTypeKey(const QChar& character, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    explicit AutoTypeKey(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    void exec(AutoTypeExecutor* executor) const override;

    const QChar character;
    const Qt::Key key = Qt::Key_unknown;
    const Qt::KeyboardModifiers modifiers;
};

class KEEPASSXC_EXPORT AutoTypeDelay : public AutoTypeAction
{
public:
    explicit AutoTypeDelay(int delayMs, bool setExecDelay = false);
    void exec(AutoTypeExecutor* executor) const override;

    const int delayMs;
    const bool setExecDelay;
};

class KEEPASSXC_EXPORT AutoTypeClearField : public AutoTypeAction
{
public:
    void exec(AutoTypeExecutor* executor) const override;
};

class KEEPASSXC_EXPORT AutoTypeExecutor
{
public:
    virtual ~AutoTypeExecutor() = default;
    virtual void execType(const AutoTypeKey* action) = 0;
    virtual void execClearField(const AutoTypeClearField* action) = 0;

    int execDelayMs = 25;
};

#endif // KEEPASSX_AUTOTYPEACTION_H
