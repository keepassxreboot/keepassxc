/*
 *  Copyright (C) 2009-2010 Jeff Gibbons
 *  Copyright (C) 2005-2008 Tarek Saidi <tarek.saidi@arcor.de>
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

#ifndef KEEPASSX_AUTOTYPEMAC_H
#define KEEPASSX_AUTOTYPEMAC_H

#include <QtCore/QtPlugin>
#include <Carbon/Carbon.h>

#include "autotype/AutoTypePlatformPlugin.h"
#include "autotype/AutoTypeAction.h"
#include "core/Global.h"

typedef quint32    KeySym;
#define NoSymbol  static_cast<KeySym>(0)
#define NoKeycode static_cast<uint16>(-1)

struct KeycodeWithMods {
    uint16 keycode;
    uint16 mods;
};

class AutoTypePlatformMac : public QObject, public AutoTypePlatformInterface
{
    Q_OBJECT
    Q_INTERFACES(AutoTypePlatformInterface)

public:
    AutoTypePlatformMac();

    QStringList windowTitles();
    WId activeWindow();
    QString activeWindowTitle();
    bool registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers);
    void unregisterGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers);
    int platformEventFilter(void* event);
    int initialTimeout();
    AutoTypeExecutor* createExecutor();

    void sendUnicode(KeySym keysym);
    void sendKeycode(KeycodeWithMods keycodeWithMods);
    KeycodeWithMods keyToKeycodeWithMods(Qt::Key key);

Q_SIGNALS:
    void globalShortcutTriggered();

private:
    QStringList getTargetWindowInfo(pid_t *pidPtr, WId *windowNumberPtr);
    uint qtToNativeModifiers(Qt::KeyboardModifiers modifiers);
    void flushUnicode();
    void keyDownUp(CGEventRef theEvent);
    void sleepTime(int msec);
    inline void sleepKeyStrokeDelay(){ sleepTime(5); };
    static OSStatus hotKeyHandler(EventHandlerCallRef,
                                  EventRef, void *userData);
    pid_t getKeepassxPID();

    bool first;
    bool onlySendKeycodes;
    bool inGlobalAutoType;
    int globalKey;
    uint globalMod;
    pid_t keepassxPID;
    EventHotKeyRef hotKeyRef;
    EventHotKeyID  hotKeyID;

    static bool inHotKeyEvent;
    static CGEventRef keyEvent;

private:
    static void initUnicodeToKeycodeWithModsMap();
    static void processToFront(pid_t pid);
    static KeycodeWithMods keysymToKeycodeWithMods2(KeySym keysym);
    static Boolean isFrontProcess(pid_t pid);
    static pid_t getKeepassxPID2();
};


class AutoTypeExecturorMac : public AutoTypeExecutor
{
public:
    explicit AutoTypeExecturorMac(AutoTypePlatformMac* platform);

    void execChar(AutoTypeChar* action);
    void execKey(AutoTypeKey* action);

private:
    AutoTypePlatformMac* const m_platform;
};

#endif // KEEPASSX_AUTOTYPEMAC_H
