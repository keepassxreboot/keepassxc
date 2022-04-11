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

#include "core/Global.h"

class AutoTypeExecutor;

class KEEPASSXC_EXPORT AutoTypeAction
{
public:
    class Result
    {
    public:
        Result()
            : m_isOk(false)
            , m_canRetry(false)
            , m_error(QString())
        {
        }

        static Result Ok()
        {
            return {true, false, QString()};
        }

        static Result Retry(const QString& error)
        {
            return {false, true, error};
        }

        static Result Failed(const QString& error)
        {
            return {false, false, error};
        }

        bool isOk() const
        {
            return m_isOk;
        }

        bool canRetry() const
        {
            return m_canRetry;
        }

        const QString& errorString() const
        {
            return m_error;
        }

    private:
        bool m_isOk;
        bool m_canRetry;
        QString m_error;

        Result(bool isOk, bool canRetry, const QString& error)
            : m_isOk(isOk)
            , m_canRetry(canRetry)
            , m_error(error)
        {
        }
    };

    AutoTypeAction() = default;
    virtual Result exec(AutoTypeExecutor* executor) const = 0;
    virtual ~AutoTypeAction() = default;
};

class KEEPASSXC_EXPORT AutoTypeKey : public AutoTypeAction
{
public:
    explicit AutoTypeKey(const QChar& character, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    explicit AutoTypeKey(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    Result exec(AutoTypeExecutor* executor) const override;

    const QChar character;
    const Qt::Key key = Qt::Key_unknown;
    const Qt::KeyboardModifiers modifiers;
};

class KEEPASSXC_EXPORT AutoTypeDelay : public AutoTypeAction
{
public:
    explicit AutoTypeDelay(int delayMs, bool setExecDelay = false);
    Result exec(AutoTypeExecutor* executor) const override;

    const int delayMs;
    const bool setExecDelay;
};

class KEEPASSXC_EXPORT AutoTypeClearField : public AutoTypeAction
{
public:
    Result exec(AutoTypeExecutor* executor) const override;
};

class KEEPASSXC_EXPORT AutoTypeBegin : public AutoTypeAction
{
public:
    Result exec(AutoTypeExecutor* executor) const override;
};

class KEEPASSXC_EXPORT AutoTypeExecutor
{
public:
    enum class Mode
    {
        NORMAL,
        VIRTUAL
    };

    virtual ~AutoTypeExecutor() = default;
    virtual AutoTypeAction::Result execBegin(const AutoTypeBegin* action) = 0;
    virtual AutoTypeAction::Result execType(const AutoTypeKey* action) = 0;
    virtual AutoTypeAction::Result execClearField(const AutoTypeClearField* action) = 0;

    int execDelayMs = 25;
    Mode mode = Mode::NORMAL;
    QString error;
};

class KEEPASSXC_EXPORT AutoTypeMode : public AutoTypeAction
{
public:
    AutoTypeMode(AutoTypeExecutor::Mode mode = AutoTypeExecutor::Mode::NORMAL);
    Result exec(AutoTypeExecutor* executor) const override;

    const AutoTypeExecutor::Mode mode;
};

#endif // KEEPASSX_AUTOTYPEACTION_H
