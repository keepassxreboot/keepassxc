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

#include "AutoTypeXCB.h"
#include "KeySymMap.h"
#include "core/Tools.h"

#include <time.h>
#include <xcb/xcb.h>

bool AutoTypePlatformX11::m_catchXErrors = false;
bool AutoTypePlatformX11::m_xErrorOccurred = false;
int (*AutoTypePlatformX11::m_oldXErrorHandler)(Display*, XErrorEvent*) = nullptr;

AutoTypePlatformX11::AutoTypePlatformX11()
{
    m_dpy = QX11Info::display();
    m_rootWindow = QX11Info::appRootWindow();

    m_atomWmState = XInternAtom(m_dpy, "WM_STATE", True);
    m_atomWmName = XInternAtom(m_dpy, "WM_NAME", True);
    m_atomNetWmName = XInternAtom(m_dpy, "_NET_WM_NAME", True);
    m_atomString = XInternAtom(m_dpy, "STRING", True);
    m_atomUtf8String = XInternAtom(m_dpy, "UTF8_STRING", True);
    m_atomNetActiveWindow = XInternAtom(m_dpy, "_NET_ACTIVE_WINDOW", True);

    m_classBlacklist << "desktop_window"
                     << "gnome-panel"; // Gnome
    m_classBlacklist << "kdesktop"
                     << "kicker"; // KDE 3
    m_classBlacklist << "Plasma"; // KDE 4
    m_classBlacklist << "plasmashell"; // KDE 5
    m_classBlacklist << "xfdesktop"
                     << "xfce4-panel"; // Xfce 4

    m_currentGlobalKey = static_cast<Qt::Key>(0);
    m_currentGlobalModifiers = 0;

    m_keysymTable = nullptr;
    m_xkb = nullptr;
    m_remapKeycode = 0;
    m_currentRemapKeysym = NoSymbol;
    m_modifierMask = ControlMask | ShiftMask | Mod1Mask | Mod4Mask;

    m_loaded = true;

    updateKeymap();
}

bool AutoTypePlatformX11::isAvailable()
{
    int ignore;

    if (!XQueryExtension(m_dpy, "XInputExtension", &ignore, &ignore, &ignore)) {
        return false;
    }

    if (!XQueryExtension(m_dpy, "XTEST", &ignore, &ignore, &ignore)) {
        return false;
    }

    if (!m_xkb) {
        XkbDescPtr kbd = getKeyboard();

        if (!kbd) {
            return false;
        }

        XkbFreeKeyboard(kbd, XkbAllComponentsMask, True);
    }

    return true;
}

void AutoTypePlatformX11::unload()
{
    // Restore the KeyboardMapping to its original state.
    if (m_currentRemapKeysym != NoSymbol) {
        AddKeysym(NoSymbol);
    }

    if (m_keysymTable) {
        XFree(m_keysymTable);
    }

    if (m_xkb) {
        XkbFreeKeyboard(m_xkb, XkbAllComponentsMask, True);
    }

    m_loaded = false;
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
        Window* children = nullptr;
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
    XGrabKey(m_dpy, keycode, nativeModifiers, m_rootWindow, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(m_dpy, keycode, nativeModifiers | Mod2Mask, m_rootWindow, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(m_dpy, keycode, nativeModifiers | LockMask, m_rootWindow, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(m_dpy, keycode, nativeModifiers | Mod2Mask | LockMask, m_rootWindow, True, GrabModeAsync, GrabModeAsync);
    stopCatchXErrors();

    if (!m_xErrorOccurred) {
        m_currentGlobalKey = key;
        m_currentGlobalModifiers = modifiers;
        m_currentGlobalKeycode = keycode;
        m_currentGlobalNativeModifiers = nativeModifiers;
        return true;
    } else {
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
        nativeModifiers |= Mod1Mask;
    }
    if (modifiers & Qt::MetaModifier) {
        nativeModifiers |= Mod4Mask;
    }

    return nativeModifiers;
}

void AutoTypePlatformX11::unregisterGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    KeyCode keycode = XKeysymToKeycode(m_dpy, charToKeySym(key));
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
    xcb_generic_event_t* genericEvent = static_cast<xcb_generic_event_t*>(event);
    quint8 type = genericEvent->response_type & 0x7f;

    if (type == XCB_KEY_PRESS || type == XCB_KEY_RELEASE) {
        xcb_key_press_event_t* keyPressEvent = static_cast<xcb_key_press_event_t*>(event);
        if (keyPressEvent->detail == m_currentGlobalKeycode
            && (keyPressEvent->state & m_modifierMask) == m_currentGlobalNativeModifiers
            && (!QApplication::activeWindow() || QApplication::activeWindow()->isMinimized())
            && m_loaded) {
            if (type == XCB_KEY_PRESS) {
                emit globalShortcutTriggered();
            }

            return 1;
        }
    } else if (type == XCB_MAPPING_NOTIFY) {
        xcb_mapping_notify_event_t* mappingNotifyEvent = static_cast<xcb_mapping_notify_event_t*>(event);
        if (mappingNotifyEvent->request == XCB_MAPPING_KEYBOARD
            || mappingNotifyEvent->request == XCB_MAPPING_MODIFIER) {
            XMappingEvent xMappingEvent;
            memset(&xMappingEvent, 0, sizeof(xMappingEvent));
            xMappingEvent.type = MappingNotify;
            xMappingEvent.display = m_dpy;
            if (mappingNotifyEvent->request == XCB_MAPPING_KEYBOARD) {
                xMappingEvent.request = MappingKeyboard;
            } else {
                xMappingEvent.request = MappingModifier;
            }
            xMappingEvent.first_keycode = mappingNotifyEvent->first_keycode;
            xMappingEvent.count = mappingNotifyEvent->count;
            XRefreshKeyboardMapping(&xMappingEvent);
            updateKeymap();
        }
    }

    return -1;
}

AutoTypeExecutor* AutoTypePlatformX11::createExecutor()
{
    return new AutoTypeExecutorX11(this);
}

QString AutoTypePlatformX11::windowTitle(Window window, bool useBlacklist)
{
    QString title;

    Atom type;
    int format;
    unsigned long nitems;
    unsigned long after;
    unsigned char* data = nullptr;

    // the window manager spec says we should read _NET_WM_NAME first, then fall back to WM_NAME

    int retVal = XGetWindowProperty(
        m_dpy, window, m_atomNetWmName, 0, 1000, False, m_atomUtf8String, &type, &format, &nitems, &after, &data);

    if ((retVal == 0) && data) {
        title = QString::fromUtf8(reinterpret_cast<char*>(data));
    } else {
        XTextProperty textProp;
        retVal = XGetTextProperty(m_dpy, window, &textProp, m_atomWmName);
        if ((retVal != 0) && textProp.value) {
            char** textList = nullptr;
            int count;

            if (textProp.encoding == m_atomUtf8String) {
                title = QString::fromUtf8(reinterpret_cast<char*>(textProp.value));
            } else if ((XmbTextPropertyToTextList(m_dpy, &textProp, &textList, &count) == 0) && textList
                       && (count > 0)) {
                title = QString::fromLocal8Bit(textList[0]);
            } else if (textProp.encoding == m_atomString) {
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
    wmClass.res_name = nullptr;
    wmClass.res_class = nullptr;

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

    for (const QWidget* widget : widgetList) {
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
    Window* children = nullptr;
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
    int retVal = XGetWindowProperty(
        m_dpy, window, m_atomWmState, 0, 2, False, m_atomWmState, &type, &format, &nitems, &after, &data);

    bool result = false;

    if (retVal == 0 && data) {
        if (type == m_atomWmState && format == 32 && nitems > 0) {
            qint32 state = static_cast<qint32>(*data);
            result = (state != WithdrawnState);
        }

        XFree(data);
    }

    return result;
}

KeySym AutoTypePlatformX11::charToKeySym(const QChar& ch)
{
    ushort unicode = ch.unicode();

    /* first check for Latin-1 characters (1:1 mapping) */
    if ((unicode >= 0x0020 && unicode <= 0x007e) || (unicode >= 0x00a0 && unicode <= 0x00ff)) {
        return unicode;
    }

    /* mapping table generated from keysymdef.h */
    const uint* match = Tools::binaryFind(m_unicodeToKeysymKeys, m_unicodeToKeysymKeys + m_unicodeToKeysymLen, unicode);
    int index = match - m_unicodeToKeysymKeys;
    if (index != m_unicodeToKeysymLen) {
        return m_unicodeToKeysymValues[index];
    }

    if (unicode >= 0x0100) {
        return unicode | 0x01000000;
    }

    return NoSymbol;
}

KeySym AutoTypePlatformX11::keyToKeySym(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Tab:
        return XK_Tab;
    case Qt::Key_Enter:
        return XK_Return;
    case Qt::Key_Space:
        return XK_space;
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
    case Qt::Key_Shift:
        return XK_Shift_L;
    case Qt::Key_Control:
        return XK_Control_L;
    case Qt::Key_Alt:
        return XK_Alt_L;
    default:
        if (key >= Qt::Key_F1 && key <= Qt::Key_F16) {
            return XK_F1 + (key - Qt::Key_F1);
        } else {
            return NoSymbol;
        }
    }
}

/*
 * Update the keyboard and modifier mapping.
 * We need the KeyboardMapping for AddKeysym.
 * Modifier mapping is required for clearing the modifiers.
 */
void AutoTypePlatformX11::updateKeymap()
{
    int keycode, inx;
    int mod_index, mod_key;
    XModifierKeymap* modifiers;

    if (m_xkb) {
        XkbFreeKeyboard(m_xkb, XkbAllComponentsMask, True);
    }
    m_xkb = getKeyboard();

    XDisplayKeycodes(m_dpy, &m_minKeycode, &m_maxKeycode);
    if (m_keysymTable != NULL)
        XFree(m_keysymTable);
    m_keysymTable = XGetKeyboardMapping(m_dpy, m_minKeycode, m_maxKeycode - m_minKeycode + 1, &m_keysymPerKeycode);

    /* determine the keycode to use for remapped keys */
    inx = (m_remapKeycode - m_minKeycode) * m_keysymPerKeycode;
    if (m_remapKeycode == 0 || !isRemapKeycodeValid()) {
        for (keycode = m_minKeycode; keycode <= m_maxKeycode; keycode++) {
            inx = (keycode - m_minKeycode) * m_keysymPerKeycode;
            if (m_keysymTable[inx] == NoSymbol) {
                m_remapKeycode = keycode;
                m_currentRemapKeysym = NoSymbol;
                break;
            }
        }
    }

    /* determine the keycode to use for modifiers */
    modifiers = XGetModifierMapping(m_dpy);
    for (mod_index = ShiftMapIndex; mod_index <= Mod5MapIndex; mod_index++) {
        m_modifier_keycode[mod_index] = 0;
        for (mod_key = 0; mod_key < modifiers->max_keypermod; mod_key++) {
            keycode = modifiers->modifiermap[mod_index * modifiers->max_keypermod + mod_key];
            if (keycode) {
                m_modifier_keycode[mod_index] = keycode;
                break;
            }
        }
    }
    XFreeModifiermap(modifiers);

    /* Xlib needs some time until the mapping is distributed to
       all clients */
    // TODO: we should probably only sleep while in the middle of typing something
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 30 * 1000 * 1000;
    nanosleep(&ts, nullptr);
}

bool AutoTypePlatformX11::isRemapKeycodeValid()
{
    int baseKeycode = (m_remapKeycode - m_minKeycode) * m_keysymPerKeycode;
    for (int i = 0; i < m_keysymPerKeycode; i++) {
        if (m_keysymTable[baseKeycode + i] == m_currentRemapKeysym) {
            return true;
        }
    }

    return false;
}

void AutoTypePlatformX11::startCatchXErrors()
{
    Q_ASSERT(!m_catchXErrors);

    m_catchXErrors = true;
    m_xErrorOccurred = false;
    m_oldXErrorHandler = XSetErrorHandler(x11ErrorHandler);
}

void AutoTypePlatformX11::stopCatchXErrors()
{
    Q_ASSERT(m_catchXErrors);

    XSync(m_dpy, False);
    XSetErrorHandler(m_oldXErrorHandler);
    m_catchXErrors = false;
}

int AutoTypePlatformX11::x11ErrorHandler(Display* display, XErrorEvent* error)
{
    Q_UNUSED(display)
    Q_UNUSED(error)

    if (m_catchXErrors) {
        m_xErrorOccurred = true;
    }

    return 1;
}

XkbDescPtr AutoTypePlatformX11::getKeyboard()
{
    int num_devices;
    XID keyboard_id = XkbUseCoreKbd;
    XDeviceInfo* devices = XListInputDevices(m_dpy, &num_devices);
    if (!devices) {
        return nullptr;
    }

    for (int i = 0; i < num_devices; i++) {
        if (QString(devices[i].name) == "Virtual core XTEST keyboard") {
            keyboard_id = devices[i].id;
            break;
        }
    }

    XFreeDeviceList(devices);

    return XkbGetKeyboard(m_dpy, XkbCompatMapMask | XkbGeometryMask, keyboard_id);
}

// --------------------------------------------------------------------------
// The following code is taken from xvkbd 3.0 and has been slightly modified.
// --------------------------------------------------------------------------

/*
 * Insert a specified keysym on the dedicated position in the keymap
 * table.
 */
int AutoTypePlatformX11::AddKeysym(KeySym keysym)
{
    if (m_remapKeycode == 0) {
        return 0;
    }

    int inx = (m_remapKeycode - m_minKeycode) * m_keysymPerKeycode;
    m_keysymTable[inx] = keysym;
    m_currentRemapKeysym = keysym;

    XChangeKeyboardMapping(m_dpy, m_remapKeycode, m_keysymPerKeycode, &m_keysymTable[inx], 1);
    XFlush(m_dpy);
    updateKeymap();

    return m_remapKeycode;
}

/*
 * Send event to the focused window.
 * If input focus is specified explicitly, select the window
 * before send event to the window.
 */
void AutoTypePlatformX11::SendKeyEvent(unsigned keycode, bool press)
{
    XSync(m_dpy, False);
    int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(MyErrorHandler);

    XTestFakeKeyEvent(m_dpy, keycode, press, 0);
    XFlush(m_dpy);

    XSetErrorHandler(oldHandler);
}

/*
 * Send a modifier press/release event for all modifiers
 * which are set in the mask variable.
 */
void AutoTypePlatformX11::SendModifiers(unsigned int mask, bool press)
{
    int mod_index;
    for (mod_index = ShiftMapIndex; mod_index <= Mod5MapIndex; mod_index++) {
        if (mask & (1 << mod_index)) {
            SendKeyEvent(m_modifier_keycode[mod_index], press);
        }
    }
}

/*
 * Determines the keycode and modifier mask for the given
 * keysym.
 */
int AutoTypePlatformX11::GetKeycode(KeySym keysym, unsigned int* mask)
{
    int keycode = XKeysymToKeycode(m_dpy, keysym);

    if (keycode && keysymModifiers(keysym, keycode, mask)) {
        return keycode;
    }

    /* no modifier matches => resort to remapping */
    keycode = AddKeysym(keysym);
    if (keycode && keysymModifiers(keysym, keycode, mask)) {
        return keycode;
    }

    *mask = 0;
    return 0;
}

bool AutoTypePlatformX11::keysymModifiers(KeySym keysym, int keycode, unsigned int* mask)
{
    int shift, mod;
    unsigned int mods_rtrn;

    /* determine whether there is a combination of the modifiers
       (Mod1-Mod5) with or without shift which returns keysym */
    for (shift = 0; shift < 2; shift++) {
        for (mod = ControlMapIndex; mod <= Mod5MapIndex; mod++) {
            KeySym keysym_rtrn;
            *mask = (mod == ControlMapIndex) ? shift : shift | (1 << mod);
            XkbTranslateKeyCode(m_xkb, keycode, *mask, &mods_rtrn, &keysym_rtrn);
            if (keysym_rtrn == keysym) {
                return true;
            }
        }
    }

    return false;
}

/*
 * Send sequence of KeyPressed/KeyReleased events to the focused
 * window to simulate keyboard.  If modifiers (shift, control, etc)
 * are set ON, many events will be sent.
 */
void AutoTypePlatformX11::SendKey(KeySym keysym, unsigned int modifiers)
{
    if (keysym == NoSymbol) {
        qWarning("No such key: keysym=0x%lX", keysym);
        return;
    }

    int keycode;
    unsigned int wanted_mask;

    /* determine keycode and mask for the given keysym */
    keycode = GetKeycode(keysym, &wanted_mask);
    if (keycode < 8 || keycode > 255) {
        qWarning("Unable to get valid keycode for key: keysym=0x%lX", keysym);
        return;
    }
    wanted_mask |= modifiers;

    Window root, child;
    int root_x, root_y, x, y;
    unsigned int original_mask;

    XSync(m_dpy, False);
    XQueryPointer(m_dpy, m_rootWindow, &root, &child, &root_x, &root_y, &x, &y, &original_mask);

    // modifiers that need to be pressed but aren't
    unsigned int press_mask = wanted_mask & ~original_mask;

    // modifiers that are pressed but maybe shouldn't
    unsigned int release_check_mask = original_mask & ~wanted_mask;

    // modifiers we need to release before sending the keycode
    unsigned int release_mask = 0;

    if (!modifiers) {
        // check every release_check_mask individually if it affects the keysym we would generate
        // if it doesn't we probably don't need to release it
        for (int mod_index = ShiftMapIndex; mod_index <= Mod5MapIndex; mod_index++) {
            if (release_check_mask & (1 << mod_index)) {
                unsigned int mods_rtrn;
                KeySym keysym_rtrn;
                XkbTranslateKeyCode(m_xkb, keycode, wanted_mask | (1 << mod_index), &mods_rtrn, &keysym_rtrn);

                if (keysym_rtrn != keysym) {
                    release_mask |= (1 << mod_index);
                }
            }
        }

        // finally check if the combination of pressed modifiers that we chose to ignore affects the keysym
        unsigned int mods_rtrn;
        KeySym keysym_rtrn;
        XkbTranslateKeyCode(
            m_xkb, keycode, wanted_mask | (release_check_mask & ~release_mask), &mods_rtrn, &keysym_rtrn);
        if (keysym_rtrn != keysym) {
            // oh well, release all the modifiers we don't want
            release_mask = release_check_mask;
        }
    } else {
        release_mask = release_check_mask;
    }

    /* set modifiers mask */
    if ((release_mask | press_mask) & LockMask) {
        SendModifiers(LockMask, true);
        SendModifiers(LockMask, false);
    }
    SendModifiers(release_mask & ~LockMask, false);
    SendModifiers(press_mask & ~LockMask, true);

    /* press and release release key */
    SendKeyEvent(keycode, true);
    SendKeyEvent(keycode, false);

    /* restore previous modifiers mask */
    SendModifiers(press_mask & ~LockMask, false);
    SendModifiers(release_mask & ~LockMask, true);
    if ((release_mask | press_mask) & LockMask) {
        SendModifiers(LockMask, true);
        SendModifiers(LockMask, false);
    }
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

AutoTypeExecutorX11::AutoTypeExecutorX11(AutoTypePlatformX11* platform)
    : m_platform(platform)
{
}

void AutoTypeExecutorX11::execChar(AutoTypeChar* action)
{
    m_platform->SendKey(m_platform->charToKeySym(action->character));
}

void AutoTypeExecutorX11::execKey(AutoTypeKey* action)
{
    m_platform->SendKey(m_platform->keyToKeySym(action->key));
}

void AutoTypeExecutorX11::execClearField(AutoTypeClearField* action = nullptr)
{
    Q_UNUSED(action);

    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 25 * 1000 * 1000;

    m_platform->SendKey(m_platform->keyToKeySym(Qt::Key_Home), static_cast<unsigned int>(ControlMask));
    nanosleep(&ts, nullptr);

    m_platform->SendKey(m_platform->keyToKeySym(Qt::Key_End), static_cast<unsigned int>(ControlMask | ShiftMask));
    nanosleep(&ts, nullptr);

    m_platform->SendKey(m_platform->keyToKeySym(Qt::Key_Backspace));
    nanosleep(&ts, nullptr);
}

bool AutoTypePlatformX11::raiseWindow(WId window)
{
    if (m_atomNetActiveWindow == None) {
        return false;
    }

    XRaiseWindow(m_dpy, window);

    XEvent event;
    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.window = window;
    event.xclient.message_type = m_atomNetActiveWindow;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1; // FromApplication
    event.xclient.data.l[1] = QX11Info::appUserTime();
    QWidget* activeWindow = QApplication::activeWindow();
    if (activeWindow) {
        event.xclient.data.l[2] = activeWindow->internalWinId();
    } else {
        event.xclient.data.l[2] = 0;
    }
    event.xclient.data.l[3] = 0;
    event.xclient.data.l[4] = 0;
    XSendEvent(m_dpy, m_rootWindow, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XFlush(m_dpy);

    return true;
}
