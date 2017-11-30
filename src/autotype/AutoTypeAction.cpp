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

#include <QEventLoop>
#include <QObject>

#include "autotype/AutoTypePlatformPlugin.h"
#include "autotype/AutoTypePickCharsDialog.h"
#include "core/Config.h"
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


AutoTypePickChars::AutoTypePickChars(QString string, AutoTypePlatformInterface* plugin)
    : string(string), plugin(plugin)
{
}

AutoTypeAction* AutoTypePickChars::clone()
{
    return new AutoTypePickChars(string, plugin);
}

void AutoTypePickChars::accept(AutoTypeExecutor* executor)
{
    executor->execPickChars(this);
}


void AutoTypeExecutor::execDelay(AutoTypeDelay* action)
{
    Tools::wait(action->delayMs);
}

void AutoTypeExecutor::execClearField(AutoTypeClearField* action)
{
    Q_UNUSED(action);
}

void AutoTypeExecutor::execPickChars(AutoTypePickChars* action)
{
    // save active window
    WId previousWindow = action->plugin->activeWindow();

    // create and show char picker dialog
    AutoTypePickCharsDialog* pickCharsDialog = new AutoTypePickCharsDialog(action->string);
#if defined(Q_OS_MAC)
    action->plugin->raiseOwnWindow();
    Tools::wait(500);
#endif
    pickCharsDialog->show();
    pickCharsDialog->activateWindow();

    // wait for dialog to finish
    QEventLoop loop;
    QObject::connect(pickCharsDialog, SIGNAL(accepted()), &loop, SLOT(quit()));
    QObject::connect(pickCharsDialog, SIGNAL(rejected()), &loop, SLOT(quit()));
    loop.exec();

    // restore previous active window
    action->plugin->raiseWindow(previousWindow);

    // enter chars
    QString chars = pickCharsDialog->chars();
    int autoTypeDelay = config()->get("AutoTypeDelay").toInt();
    for (const QChar& ch : chars) {
        AutoTypeDelay* delayAction = new AutoTypeDelay(autoTypeDelay);
        execDelay(delayAction);
        delete delayAction;

        if (ch == '\n') {
            AutoTypeKey* keyAction = new AutoTypeKey(Qt::Key_Enter);
            execKey(keyAction);
            delete keyAction;
        }
        else if (ch == '\t') {
            AutoTypeKey* keyAction = new AutoTypeKey(Qt::Key_Tab);
            execKey(keyAction);
            delete keyAction;
        }
        else {
            AutoTypeChar* charAction = new AutoTypeChar(ch);
            execChar(charAction);
            delete charAction;
        }
    }
}
