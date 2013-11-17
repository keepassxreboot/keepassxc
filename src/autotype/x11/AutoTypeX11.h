/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2000-2008 Tom Sato <VEF00200@nifty.ne.jp>
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

#ifndef KEEPASSX_AUTOTYPEX11_H
#define KEEPASSX_AUTOTYPEX11_H

#include <QApplication>
#include <QSet>
#include <QtPlugin>
#include <QWidget>
#include <QX11Info>

#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>

#include "autotype/AutoTypePlatformPlugin.h"
#include "autotype/AutoTypeAction.h"
#include "core/Global.h"

class AutoTypePlatformX11 : public QObject, public AutoTypePlatformInterface
{
    Q_OBJECT
    Q_INTERFACES(AutoTypePlatformInterface)

public:
    AutoTypePlatformX11();
    QStringList windowTitles();
    WId activeWindow();
    QString activeWindowTitle();
    bool registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers);
    void unregisterGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers);
    int platformEventFilter(void* event);
    int initialTimeout();
    AutoTypeExecutor* createExecutor();

    KeySym charToKeySym(const QChar& ch);
    KeySym keyToKeySym(Qt::Key key);

    void SendKeyPressedEvent(KeySym keysym, unsigned int shift = 0);

Q_SIGNALS:
    void globalShortcutTriggered();

private:
    QString windowTitle(Window window, bool useBlacklist);
    QStringList windowTitlesRecursive(Window window);
    QString windowClassName(Window window);
    QList<Window> widgetsToX11Windows(const QWidgetList& widgetList);
    bool isTopLevelWindow(Window window);
    uint qtToNativeModifiers(Qt::KeyboardModifiers modifiers);
    void startCatchXErrors();
    void stopCatchXErrors();
    static int x11ErrorHandler(Display* display, XErrorEvent* error);

    void updateKeymap();
    int AddKeysym(KeySym keysym, bool top);
    void AddModifier(KeySym keysym);
    void ReadKeymap();
    void SendEvent(XKeyEvent* event);
    static int MyErrorHandler(Display* my_dpy, XErrorEvent* event);

    Display* m_dpy;
    Window m_rootWindow;
    Atom m_atomWmState;
    Atom m_atomWmName;
    Atom m_atomNetWmName;
    Atom m_atomString;
    Atom m_atomUtf8String;
    QSet<QString> m_classBlacklist;
    Qt::Key m_currentGlobalKey;
    Qt::KeyboardModifiers m_currentGlobalModifiers;
    uint m_currentGlobalKeycode;
    uint m_currentGlobalNativeModifiers;
    int m_modifierMask;
    static bool m_catchXErrors;
    static bool m_xErrorOccured;
    static int (*m_oldXErrorHandler)(Display*, XErrorEvent*);

    KeySym* m_keysymTable;
    int m_minKeycode;
    int m_maxKeycode;
    int m_keysymPerKeycode;
    int m_altMask;
    int m_metaMask;
    int m_altgrMask;
    /* index of the XGetKeyboardMapping for the AltGr key */
    int inx_altgr;
    KeySym m_altgrKeysym;
};

class AutoTypeExecturorX11 : public AutoTypeExecutor
{
public:
    explicit AutoTypeExecturorX11(AutoTypePlatformX11* platform);

    void execChar(AutoTypeChar* action);
    void execKey(AutoTypeKey* action);

private:
    AutoTypePlatformX11* const m_platform;
};

#endif // KEEPASSX_AUTOTYPEX11_H
