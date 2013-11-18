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

#include "AutoTypeX11.h"

bool AutoTypePlatformX11::m_catchXErrors = false;
bool AutoTypePlatformX11::m_xErrorOccured = false;
int (*AutoTypePlatformX11::m_oldXErrorHandler)(Display*, XErrorEvent*) = Q_NULLPTR;

AutoTypePlatformX11::AutoTypePlatformX11()
{
    m_dpy = QX11Info::display();
    m_rootWindow = QX11Info::appRootWindow();

    m_atomWmState = XInternAtom(m_dpy, "WM_STATE", true);
    m_atomWmName = XInternAtom(m_dpy, "WM_NAME", true);
    m_atomNetWmName = XInternAtom(m_dpy, "_NET_WM_NAME", true);
    m_atomString = XInternAtom(m_dpy, "STRING", true);
    m_atomUtf8String = XInternAtom(m_dpy, "UTF8_STRING", true);

    m_classBlacklist << "desktop_window" << "gnome-panel"; // Gnome
    m_classBlacklist << "kdesktop" << "kicker"; // KDE 3
    m_classBlacklist << "Plasma"; // KDE 4
    m_classBlacklist << "xfdesktop" << "xfce4-panel"; // Xfce 4

    m_currentGlobalKey = static_cast<Qt::Key>(0);
    m_currentGlobalModifiers = 0;

    m_keysymTable = Q_NULLPTR;
    m_altMask = 0;
    m_metaMask = 0;
    m_altgrMask = 0;
    m_altgrKeysym = NoSymbol;

    updateKeymap();
}

QStringList AutoTypePlatformX11::windowTitles()
{
    return windowTitlesRecursive(m_rootWindow);
}

WId AutoTypePlatformX11::activeWindow()
{
    Window window;
    int revert_to_return;
    XGetInputFocus(m_dpy, &window, &revert_to_return);

    int tree;
    do {
        if (isTopLevelWindow(window)) {
            break;
        }

        Window root;
        Window parent;
        Window* children = Q_NULLPTR;
        unsigned int numChildren;
        tree = XQueryTree(m_dpy, window, &root, &parent, &children, &numChildren);
        window = parent;
        if (children) {
            XFree(children);
        }
    } while (tree && window);

    return window;
}

QString AutoTypePlatformX11::activeWindowTitle()
{
    return windowTitle(activeWindow(), true);
}

bool AutoTypePlatformX11::registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    int keycode = XKeysymToKeycode(m_dpy, charToKeySym(key));
    uint nativeModifiers = qtToNativeModifiers(modifiers);

    startCatchXErrors();
    XGrabKey(m_dpy, keycode, nativeModifiers, m_rootWindow, true, GrabModeAsync, GrabModeAsync);
    XGrabKey(m_dpy, keycode, nativeModifiers | Mod2Mask, m_rootWindow, true, GrabModeAsync,
             GrabModeAsync);
    XGrabKey(m_dpy, keycode, nativeModifiers | LockMask, m_rootWindow, true, GrabModeAsync,
             GrabModeAsync);
    XGrabKey(m_dpy, keycode, nativeModifiers | Mod2Mask | LockMask, m_rootWindow, true,
             GrabModeAsync, GrabModeAsync);
    stopCatchXErrors();

    if (!m_xErrorOccured) {
        m_currentGlobalKey = key;
        m_currentGlobalModifiers = modifiers;
        m_currentGlobalKeycode = keycode;
        m_currentGlobalNativeModifiers = nativeModifiers;
        return true;
    }
    else {
        unregisterGlobalShortcut(key, modifiers);
        return false;
    }
}

uint AutoTypePlatformX11::qtToNativeModifiers(Qt::KeyboardModifiers modifiers)
{
    uint nativeModifiers = 0;

    if (modifiers & Qt::ShiftModifier) {
        nativeModifiers |= ShiftMask;
    }
    if (modifiers & Qt::ControlModifier) {
        nativeModifiers |= ControlMask;
    }
    if (modifiers & Qt::AltModifier) {
        nativeModifiers |= m_altMask;
    }
    if (modifiers & Qt::MetaModifier) {
        nativeModifiers |= m_metaMask;
    }

    return nativeModifiers;
}

void AutoTypePlatformX11::unregisterGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    KeyCode keycode = XKeysymToKeycode(m_dpy, keyToKeySym(key));
    uint nativeModifiers = qtToNativeModifiers(modifiers);

    XUngrabKey(m_dpy, keycode, nativeModifiers, m_rootWindow);
    XUngrabKey(m_dpy, keycode, nativeModifiers | Mod2Mask, m_rootWindow);
    XUngrabKey(m_dpy, keycode, nativeModifiers | LockMask, m_rootWindow);
    XUngrabKey(m_dpy, keycode, nativeModifiers | Mod2Mask | LockMask, m_rootWindow);

    m_currentGlobalKey = static_cast<Qt::Key>(0);
    m_currentGlobalModifiers = 0;
    m_currentGlobalKeycode = 0;
    m_currentGlobalNativeModifiers = 0;
}

int AutoTypePlatformX11::platformEventFilter(void* event)
{
    XEvent* xevent = static_cast<XEvent*>(event);

    if ((xevent->type == KeyPress || xevent->type == KeyRelease)
            && m_currentGlobalKey
            && xevent->xkey.keycode == m_currentGlobalKeycode
            && (xevent->xkey.state & m_modifierMask) == m_currentGlobalNativeModifiers
            && !QApplication::focusWidget()) {
        if (xevent->type == KeyPress) {
            Q_EMIT globalShortcutTriggered();
        }
        return 1;
    }
    if (xevent->type == MappingNotify) {
        updateKeymap();
    }

    return -1;
}

AutoTypeExecutor* AutoTypePlatformX11::createExecutor()
{
    return new AutoTypeExecturorX11(this);
}

QString AutoTypePlatformX11::windowTitle(Window window, bool useBlacklist)
{
    QString title;

    Atom type;
    int format;
    unsigned long nitems;
    unsigned long after;
    unsigned char* data = Q_NULLPTR;

    int retVal = XGetWindowProperty(m_dpy, window, m_atomNetWmName, 0, 1000, false, m_atomUtf8String,
                                    &type, &format, &nitems, &after, &data);

    if (retVal != 0 && data) {
        title = QString::fromUtf8(reinterpret_cast<char*>(data));
    }
    else {
        XTextProperty textProp;
        retVal = XGetTextProperty(m_dpy, window, &textProp, m_atomWmName);
        if (retVal != 0 && textProp.value) {
            char** textList = Q_NULLPTR;
            int count;

            if (textProp.encoding == m_atomUtf8String) {
                title = QString::fromUtf8(reinterpret_cast<char*>(textProp.value));
            }
            else if (XmbTextPropertyToTextList(m_dpy, &textProp, &textList, &count) == 0 && textList && count > 0) {
                title = QString::fromLocal8Bit(textList[0]);
            }
            else if (textProp.encoding == m_atomString) {
                title = QString::fromLocal8Bit(reinterpret_cast<char*>(textProp.value));
            }

            if (textList) {
                XFreeStringList(textList);
            }
        }

        if (textProp.value) {
            XFree(textProp.value);
        }
    }

    if (data) {
        XFree(data);
    }

    if (useBlacklist && !title.isEmpty()) {
        if (window == m_rootWindow) {
            return QString();
        }

        QString className = windowClassName(window);
        if (m_classBlacklist.contains(className)) {
            return QString();
        }

        QList<Window> keepassxWindows = widgetsToX11Windows(QApplication::topLevelWidgets());
        if (keepassxWindows.contains(window)) {
            return QString();
        }
    }

    return title;
}

QString AutoTypePlatformX11::windowClassName(Window window)
{
    QString className;

    XClassHint wmClass;
    wmClass.res_name = Q_NULLPTR;
    wmClass.res_class = Q_NULLPTR;

    if (XGetClassHint(m_dpy, window, &wmClass) && wmClass.res_name) {
        className = QString::fromLocal8Bit(wmClass.res_name);
    }
    if (wmClass.res_name) {
        XFree(wmClass.res_name);
    }
    if (wmClass.res_class) {
        XFree(wmClass.res_class);
    }

    return className;
}

QList<Window> AutoTypePlatformX11::widgetsToX11Windows(const QWidgetList& widgetList)
{
    QList<Window> windows;

    Q_FOREACH (const QWidget* widget, widgetList) {
        windows.append(widget->effectiveWinId());
    }

    return windows;
}

QStringList AutoTypePlatformX11::windowTitlesRecursive(Window window)
{
    QStringList titles;

    if (isTopLevelWindow(window)) {
        QString title = windowTitle(window, true);
        if (!title.isEmpty()) {
            titles.append(title);
        }
    }

    Window root;
    Window parent;
    Window* children = Q_NULLPTR;
    unsigned int numChildren;
    if (XQueryTree(m_dpy, window, &root, &parent, &children, &numChildren) && children) {
        for (uint i = 0; i < numChildren; i++) {
            titles.append(windowTitlesRecursive(children[i]));
        }
    }
    if (children) {
        XFree(children);
    }

    return titles;
}

bool AutoTypePlatformX11::isTopLevelWindow(Window window)
{
    Atom type = None;
    int format;
    unsigned long nitems;
    unsigned long after;
    unsigned char* data = Q_NULLPTR;
    int retVal = XGetWindowProperty(m_dpy, window, m_atomWmState, 0, 0, false, AnyPropertyType, &type, &format,
                                    &nitems, &after, &data);
    if (data) {
        XFree(data);
    }

    return (retVal == 0) && type;
}

KeySym AutoTypePlatformX11::charToKeySym(const QChar& ch)
{
    ushort unicode = ch.unicode();

    /* first check for Latin-1 characters (1:1 mapping) */
    if ((unicode >= 0x0020 && unicode <= 0x007e)
            || (unicode >= 0x00a0 && unicode <= 0x00ff)) {
        return unicode;
    }
    else if (unicode >= 0x0100) {
        return unicode | 0x01000000;
    }
    else {
        return NoSymbol;
    }
}

KeySym AutoTypePlatformX11::keyToKeySym(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Tab:
        return XK_Tab;
    case Qt::Key_Enter:
        return XK_Return;
    case Qt::Key_Up:
        return XK_Up;
    case Qt::Key_Down:
        return XK_Down;
    case Qt::Key_Left:
        return XK_Left;
    case Qt::Key_Right:
        return XK_Right;
    case Qt::Key_Insert:
        return XK_Insert;
    case Qt::Key_Delete:
        return XK_Delete;
    case Qt::Key_Home:
        return XK_Home;
    case Qt::Key_End:
        return XK_End;
    case Qt::Key_PageUp:
        return XK_Page_Up;
    case Qt::Key_PageDown:
        return XK_Page_Down;
    case Qt::Key_Backspace:
        return XK_BackSpace;
    case Qt::Key_Pause:
        return XK_Break;
    case Qt::Key_CapsLock:
        return XK_Caps_Lock;
    case Qt::Key_Escape:
        return XK_Escape;
    case Qt::Key_Help:
        return XK_Help;
    case Qt::Key_NumLock:
        return XK_Num_Lock;
    case Qt::Key_Print:
        return XK_Print;
    case Qt::Key_ScrollLock:
        return XK_Scroll_Lock;
    default:
        if (key >= Qt::Key_F1 && key <= Qt::Key_F16) {
            return XK_F1 + (key - Qt::Key_F1);
        }
        else {
            return NoSymbol;
        }
    }
}

void AutoTypePlatformX11::updateKeymap()
{
    ReadKeymap();

    if (!m_altgrMask) {
        AddModifier(XK_Mode_switch);
    }
    if (!m_metaMask) {
        m_metaMask = Mod4Mask;
    }

    m_modifierMask = ControlMask | ShiftMask | m_altMask | m_metaMask;

    // TODO: figure out why this breaks after the first global auto-type
    /*if (m_currentGlobalKey && m_currentGlobalModifiers) {
        unregisterGlobalShortcut(m_currentGlobalKey, m_currentGlobalModifiers);
        registerGlobalShortcut(m_currentGlobalKey, m_currentGlobalModifiers);
    }*/
}

void AutoTypePlatformX11::startCatchXErrors()
{
    Q_ASSERT(!m_catchXErrors);

    m_catchXErrors = true;
    m_xErrorOccured = false;
    m_oldXErrorHandler = XSetErrorHandler(x11ErrorHandler);
}

void AutoTypePlatformX11::stopCatchXErrors()
{
    Q_ASSERT(m_catchXErrors);

    XSync(m_dpy, false);
    XSetErrorHandler(m_oldXErrorHandler);
    m_catchXErrors = false;
}

int AutoTypePlatformX11::x11ErrorHandler(Display* display, XErrorEvent* error)
{
    Q_UNUSED(display)
    Q_UNUSED(error)

    if (m_catchXErrors) {
        m_xErrorOccured = true;
    }

    return 1;
}

// --------------------------------------------------------------------------
// The following code is taken from xvkbd 3.0 and has been slightly modified.
// --------------------------------------------------------------------------

/*
 * Insert a specified keysym to unused position in the keymap table.
 * This will be called to add required keysyms on-the-fly.
 * if the second parameter is TRUE, the keysym will be added to the
 * non-shifted position - this may be required for modifier keys
 * (e.g. Mode_switch) and some special keys (e.g. F20).
 */
int AutoTypePlatformX11::AddKeysym(KeySym keysym, bool top)
{
    int keycode, pos, max_pos, inx, phase;

    if (top) {
        max_pos = 0;
    } else {
        max_pos = m_keysymPerKeycode - 1;
        if (4 <= max_pos) max_pos = 3;
        if (2 <= max_pos && m_altgrKeysym != XK_Mode_switch) max_pos = 1;
    }

    for (phase = 0; phase < 2; phase++) {
        for (keycode = m_maxKeycode; m_minKeycode <= keycode; keycode--) {
            for (pos = max_pos; 0 <= pos; pos--) {
                inx = (keycode - m_minKeycode) * m_keysymPerKeycode;
                if ((phase != 0 || m_keysymTable[inx] == NoSymbol) && m_keysymTable[inx] < 0xFF00) {
                    /* In the first phase, to avoid modifing existing keys, */
                    /* add the keysym only to the keys which has no keysym in the first position. */
                    /* If no place fuond in the first phase, add the keysym for any keys except */
                    /* for modifier keys and other special keys */
                    if (m_keysymTable[inx + pos] == NoSymbol) {
                        m_keysymTable[inx + pos] = keysym;
                        XChangeKeyboardMapping(m_dpy, keycode, m_keysymPerKeycode, &m_keysymTable[inx], 1);
                        XFlush(m_dpy);
                        return keycode;
                    }
                }
            }
        }
    }
    qWarning("Couldn't add \"%s\" to keymap", XKeysymToString(keysym));
    return NoSymbol;
}

/*
 * Add the specified key as a new modifier.
 * This is used to use Mode_switch (AltGr) as a modifier.
 */
void AutoTypePlatformX11::AddModifier(KeySym keysym)
{
    XModifierKeymap *modifiers;
    int keycode, i, pos;

    keycode = XKeysymToKeycode(m_dpy, keysym);
    if (keycode == NoSymbol) keycode = AddKeysym(keysym, TRUE);

    modifiers = XGetModifierMapping(m_dpy);
    for (i = 7; 3 < i; i--) {
        if (modifiers->modifiermap[i * modifiers->max_keypermod] == NoSymbol
            || ((m_keysymTable[(modifiers->modifiermap[i * modifiers->max_keypermod]
            - m_minKeycode) * m_keysymPerKeycode]) == XK_ISO_Level3_Shift
            && keysym == XK_Mode_switch))
        {
            for (pos = 0; pos < modifiers->max_keypermod; pos++) {
                if (modifiers->modifiermap[i * modifiers->max_keypermod + pos] == NoSymbol) {
                    modifiers->modifiermap[i * modifiers->max_keypermod + pos] = keycode;
                    XSetModifierMapping(m_dpy, modifiers);
                    return;
                }
            }
        }
    }
    qWarning("Couldn't add \"%s\" as modifier", XKeysymToString(keysym));
}

/*
 * Read keyboard mapping and modifier mapping.
 * Keyboard mapping is used to know what keys are in shifted position.
 * Modifier mapping is required because we should know Alt and Meta
 * key are used as which modifier.
 */
void AutoTypePlatformX11::ReadKeymap()
{
    int i;
    int keycode, inx, pos;
    KeySym keysym;
    XModifierKeymap *modifiers;

    XDisplayKeycodes(m_dpy, &m_minKeycode, &m_maxKeycode);
    if (m_keysymTable != NULL) XFree(m_keysymTable);
    m_keysymTable = XGetKeyboardMapping(m_dpy,
            m_minKeycode, m_maxKeycode - m_minKeycode + 1,
            &m_keysymPerKeycode);
    for (keycode = m_minKeycode; keycode <= m_maxKeycode; keycode++) {
    /* if the first keysym is alphabet and the second keysym is NoSymbol,
        it is equivalent to pair of lowercase and uppercase alphabet */
        inx = (keycode - m_minKeycode) * m_keysymPerKeycode;
        if (m_keysymTable[inx + 1] == NoSymbol
                  && ((XK_A <= m_keysymTable[inx] && m_keysymTable[inx] <= XK_Z)
                  || (XK_a <= m_keysymTable[inx] && m_keysymTable[inx] <= XK_z)))
        {
            if (XK_A <= m_keysymTable[inx] && m_keysymTable[inx] <= XK_Z)
                m_keysymTable[inx] = m_keysymTable[inx] - XK_A + XK_a;
            m_keysymTable[inx + 1] = m_keysymTable[inx] - XK_a + XK_A;
        }
    }

    m_altMask = 0;
    m_metaMask = 0;
    m_altgrMask = 0;
    m_altgrKeysym = NoSymbol;
    modifiers = XGetModifierMapping(m_dpy);
    /* default value, used if we do not find a mapping */
    inx_altgr = 3;

    for (i = 0; i < 8; i++) {
        for (pos = 0; pos < modifiers->max_keypermod; pos++) {
            keycode = modifiers->modifiermap[i * modifiers->max_keypermod + pos];
            if (keycode < m_minKeycode || m_maxKeycode < keycode) continue;

            keysym = m_keysymTable[(keycode - m_minKeycode) * m_keysymPerKeycode];
            if (keysym == XK_Alt_L || keysym == XK_Alt_R) {
                m_altMask = 1 << i;
            } else if (keysym == XK_Meta_L || keysym == XK_Meta_R) {
                m_metaMask = 1 << i;
            } else if (keysym == XK_Mode_switch) {
                if (m_altgrKeysym == XK_ISO_Level3_Shift) {
                } else {
                    m_altgrMask = 0x0101 << i;
                    /* I don't know why, but 0x2000 was required for mod3 on my Linux box */
                    m_altgrKeysym = keysym;

                    /* set the index of the XGetKeyboardMapping for the AltGr key */
                    inx_altgr = i;
                }
            } else if (keysym == XK_ISO_Level3_Shift) {
                /* if no Mode_switch, try to use ISO_Level3_Shift instead */
                /* however, it may not work as intended - I don't know why */
                m_altgrMask = 1 << i;
                m_altgrKeysym = keysym;

                /* set the index of the XGetKeyboardMapping for the AltGr key */
                inx_altgr = i;
            }
        }
    }
    XFreeModifiermap(modifiers);
}

/*
 * Send event to the focused window.
 * If input focus is specified explicitly, select the window
 * before send event to the window.
 */
void AutoTypePlatformX11::SendEvent(XKeyEvent* event)
{
    XSync(event->display, FALSE);
    int (*oldHandler) (Display*, XErrorEvent*) = XSetErrorHandler(MyErrorHandler);

    XTestFakeKeyEvent(event->display, event->keycode, event->type == KeyPress, 0);
    XFlush(event->display);

    XSetErrorHandler(oldHandler);
}

/*
 * Send sequence of KeyPressed/KeyReleased events to the focused
 * window to simulate keyboard.  If modifiers (shift, control, etc)
 * are set ON, many events will be sent.
 */
void AutoTypePlatformX11::SendKeyPressedEvent(KeySym keysym, unsigned int shift)
{
    Window cur_focus;
    int revert_to;
    XKeyEvent event;
    int keycode;
    int phase;
    bool found;
    KeySym ks;
    unsigned int mods_rtrn = 0;
    XkbDescPtr kbd = XkbGetKeyboard (m_dpy, XkbCompatMapMask | XkbGeometryMask, XkbUseCoreKbd);

    XGetInputFocus(m_dpy, &cur_focus, &revert_to);

    found = FALSE;
    keycode = 0;

    if (keysym != NoSymbol) {
        for (phase = 0; phase < 2; phase++) {
            keycode = XKeysymToKeycode(m_dpy, keysym);
            XkbTranslateKeyCode(kbd, keycode, 0, &mods_rtrn, &ks);
            if (ks == keysym) {
                shift &= ~m_altgrMask;
                shift &= ~ShiftMask;
                found = TRUE;
                break;
            } 

            XkbTranslateKeyCode(kbd, keycode, ShiftMask, &mods_rtrn, &ks);
            if (ks == keysym) {
                 shift &= ~m_altgrMask;
                 shift |= ShiftMask;
                 found = TRUE;
                 break;
            } 

            XkbTranslateKeyCode(kbd, keycode, Mod5Mask, &mods_rtrn, &ks);
            if (ks == keysym) {
                 shift &= ~ShiftMask;
                 shift |= m_altgrMask;
                 found = TRUE;
                 break;
            } 
         
            XkbTranslateKeyCode(kbd, keycode, Mod5Mask, &mods_rtrn, &ks);
            if (ks == keysym) {
                 shift |= ShiftMask | m_altgrMask;
                 found = TRUE;
                 break;
            } 
            if (found) break;

            if (0xF000 <= keysym) {
                 /* for special keys such as function keys,
                 first try to add it in the non-shifted position of the keymap */
                 if (AddKeysym(keysym, TRUE) == NoSymbol) AddKeysym(keysym, FALSE);
            } else {
                    AddKeysym(keysym, FALSE);
            }
        }
    }

    event.display = m_dpy;
    event.window = cur_focus;
    event.root = m_rootWindow;
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = 1;
    event.y = 1;
    event.x_root = 1;
    event.y_root = 1;
    event.same_screen = TRUE;

    Window root, child;
    int root_x, root_y, x, y;
    unsigned int mask;

    XQueryPointer(m_dpy, event.root, &root, &child, &root_x, &root_y, &x, &y, &mask);

    event.type = KeyRelease;
    event.state = 0;
    if (mask & ControlMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Control_L);
        SendEvent(&event);
    }
    if (mask & m_altMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Alt_L);
        SendEvent(&event);
    }
    if (mask & m_metaMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Meta_L);
        SendEvent(&event);
    }
    if (mask & m_altgrMask) {
        event.keycode = XKeysymToKeycode(m_dpy, m_altgrKeysym);
        SendEvent(&event);
    }
    if (mask & ShiftMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Shift_L);
        SendEvent(&event);
    }
    if (mask & LockMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Caps_Lock);
        SendEvent(&event);
    }

    event.type = KeyPress;
    event.state = 0;
    if (shift & ControlMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Control_L);
        SendEvent(&event);
        event.state |= ControlMask;
    }
    if (shift & m_altMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Alt_L);
        SendEvent(&event);
        event.state |= m_altMask;
    }
    if (shift & m_metaMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Meta_L);
        SendEvent(&event);
        event.state |= m_metaMask;
    }
    if (shift & m_altgrMask) {
        event.keycode = XKeysymToKeycode(m_dpy, m_altgrKeysym);
        SendEvent(&event);
        event.state |= m_altgrMask;
    }
    if (shift & ShiftMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Shift_L);
        SendEvent(&event);
        event.state |= ShiftMask;
    }

    if (keysym != NoSymbol) {  /* send event for the key itself */
        event.keycode = found ? keycode : XKeysymToKeycode(m_dpy, keysym);
        if (event.keycode == NoSymbol) {
            if ((keysym & ~0x7f) == 0 && QChar(static_cast<char>(keysym)).isPrint())
                qWarning("No such key: %c", static_cast<char>(keysym));
            else if (XKeysymToString(keysym) != NULL)
                qWarning("No such key: keysym=%s (0x%lX)", XKeysymToString(keysym), static_cast<long>(keysym));
            else
                qWarning("No such key: keysym=0x%lX", static_cast<long>(keysym));
        } else {
            SendEvent(&event);
            event.type = KeyRelease;
            SendEvent(&event);
        }
    }

    event.type = KeyRelease;
    if (shift & ShiftMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Shift_L);
        SendEvent(&event);
        event.state &= ~ShiftMask;
    }
    if (shift & m_altgrMask) {
        event.keycode = XKeysymToKeycode(m_dpy, m_altgrKeysym);
        SendEvent(&event);
        event.state &= ~m_altgrMask;
    }
    if (shift & m_metaMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Meta_L);
        SendEvent(&event);
        event.state &= ~m_metaMask;
    }
    if (shift & m_altMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Alt_L);
        SendEvent(&event);
        event.state &= ~m_altMask;
    }
    if (shift & ControlMask) {
        event.keycode = XKeysymToKeycode(m_dpy, XK_Control_L);
        SendEvent(&event);
        event.state &= ~ControlMask;
    }
    XkbFreeKeyboard(kbd, XkbAllComponentsMask, True);
}

int AutoTypePlatformX11::MyErrorHandler(Display* my_dpy, XErrorEvent* event)
{
    char msg[200];

    if (event->error_code == BadWindow) {
        return 0;
    }
    XGetErrorText(my_dpy, event->error_code, msg, sizeof(msg) - 1);
    qWarning("X error trapped: %s, request-code=%d\n", msg, event->request_code);
    return 0;
}


AutoTypeExecturorX11::AutoTypeExecturorX11(AutoTypePlatformX11* platform)
    : m_platform(platform)
{
}

void AutoTypeExecturorX11::execChar(AutoTypeChar* action)
{
    m_platform->SendKeyPressedEvent(m_platform->charToKeySym(action->character));
}

void AutoTypeExecturorX11::execKey(AutoTypeKey* action)
{
    m_platform->SendKeyPressedEvent(m_platform->keyToKeySym(action->key));
}

int AutoTypePlatformX11::initialTimeout()
{
    return 500;
}

Q_EXPORT_PLUGIN2(keepassx-autotype-x11, AutoTypePlatformX11)
