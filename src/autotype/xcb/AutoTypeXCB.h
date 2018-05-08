/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2000-2008 Tom Sato <VEF00200@nifty.ne.jp>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_AUTOTYPEXCB_H
#define KEEPASSX_AUTOTYPEXCB_H

#include <QApplication>
#include <QSet>
#include <QWidget>
#include <QX11Info>
#include <QtPlugin>

#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

#include "autotype/AutoTypeAction.h"
#include "autotype/AutoTypePlatformPlugin.h"

#define N_MOD_INDICES (Mod5MapIndex + 1)

class AutoTypePlatformX11 : public QObject, public AutoTypePlatformInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.keepassx.AutoTypePlatformX11")
    Q_INTERFACES(AutoTypePlatformInterface)

public:
    AutoTypePlatformX11();
    bool isAvailable() override;
    void unload() override;
    QStringList windowTitles() override;
    WId activeWindow() override;
    QString activeWindowTitle() override;
    bool registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers) override;
    void unregisterGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers) override;
    int platformEventFilter(void* event) override;
    bool raiseWindow(WId window) override;
    AutoTypeExecutor* createExecutor() override;

    KeySym charToKeySym(const QChar& ch);
    KeySym keyToKeySym(Qt::Key key);

    void SendKey(KeySym keysym, unsigned int modifiers = 0);

signals:
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

    XkbDescPtr getKeyboard();
    void updateKeymap();
    bool isRemapKeycodeValid();
    int AddKeysym(KeySym keysym);
    void AddModifier(KeySym keysym);
    void SendKeyEvent(unsigned keycode, bool press);
    void SendModifiers(unsigned int mask, bool press);
    int GetKeycode(KeySym keysym, unsigned int* mask);
    bool keysymModifiers(KeySym keysym, int keycode, unsigned int* mask);

    static int MyErrorHandler(Display* my_dpy, XErrorEvent* event);

    Display* m_dpy;
    Window m_rootWindow;
    Atom m_atomWmState;
    Atom m_atomWmName;
    Atom m_atomNetWmName;
    Atom m_atomString;
    Atom m_atomUtf8String;
    Atom m_atomNetActiveWindow;
    QSet<QString> m_classBlacklist;
    Qt::Key m_currentGlobalKey;
    Qt::KeyboardModifiers m_currentGlobalModifiers;
    uint m_currentGlobalKeycode;
    uint m_currentGlobalNativeModifiers;
    int m_modifierMask;
    static bool m_catchXErrors;
    static bool m_xErrorOccurred;
    static int (*m_oldXErrorHandler)(Display*, XErrorEvent*);

    static const int m_unicodeToKeysymLen;
    static const uint m_unicodeToKeysymKeys[];
    static const uint m_unicodeToKeysymValues[];

    XkbDescPtr m_xkb;
    KeySym* m_keysymTable;
    int m_minKeycode;
    int m_maxKeycode;
    int m_keysymPerKeycode;
    /* dedicated keycode for remapped keys */
    unsigned int m_remapKeycode;
    KeySym m_currentRemapKeysym;
    KeyCode m_modifier_keycode[N_MOD_INDICES];
    bool m_loaded;
};

class AutoTypeExecutorX11 : public AutoTypeExecutor
{
public:
    explicit AutoTypeExecutorX11(AutoTypePlatformX11* platform);

    void execChar(AutoTypeChar* action) override;
    void execKey(AutoTypeKey* action) override;
    void execClearField(AutoTypeClearField* action) override;

private:
    AutoTypePlatformX11* const m_platform;
};

#endif // KEEPASSX_AUTOTYPEXCB_H
