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

#ifndef KEEPASSX_AUTOTYPEACTION_H
#define KEEPASSX_AUTOTYPEACTION_H

#include <QChar>
#include <QObject>
#include <Qt>

#include "core/Global.h"

class AutoTypeExecutor;

class KEEPASSX_EXPORT AutoTypeAction
{
public:
    virtual ~AutoTypeAction()
    {
    }
    virtual AutoTypeAction* clone() = 0;
    virtual void accept(AutoTypeExecutor* executor) = 0;
};

class KEEPASSX_EXPORT AutoTypeChar : public AutoTypeAction
{
public:
    explicit AutoTypeChar(const QChar& character);
    AutoTypeAction* clone();
    void accept(AutoTypeExecutor* executor);

    const QChar character;
};

class KEEPASSX_EXPORT AutoTypeKey : public AutoTypeAction
{
public:
    explicit AutoTypeKey(Qt::Key key);
    AutoTypeAction* clone();
    void accept(AutoTypeExecutor* executor);

    const Qt::Key key;
};

class KEEPASSX_EXPORT AutoTypeDelay : public AutoTypeAction
{
public:
    explicit AutoTypeDelay(int delayMs);
    AutoTypeAction* clone();
    void accept(AutoTypeExecutor* executor);

    const int delayMs;
};

class KEEPASSX_EXPORT AutoTypeClearField : public AutoTypeAction
{
public:
    AutoTypeClearField();
    AutoTypeAction* clone();
    void accept(AutoTypeExecutor* executor);
};

class KEEPASSX_EXPORT AutoTypeExecutor
{
public:
    virtual ~AutoTypeExecutor()
    {
    }
    virtual void execChar(AutoTypeChar* action) = 0;
    virtual void execKey(AutoTypeKey* action) = 0;
    virtual void execDelay(AutoTypeDelay* action);
    virtual void execClearField(AutoTypeClearField* action);
};

#endif // KEEPASSX_AUTOTYPEACTION_H
