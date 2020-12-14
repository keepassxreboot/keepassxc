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

    m_keysymTable = nullptr;
    m_xkb = nullptr;
    m_remapKeycode = 0;
    m_currentRemapKeysym = NoSymbol;

    m_loaded = true;

    connect(nixUtils(), &NixUtils::keymapChanged, this, [this] { updateKeymap(); });
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

/*
 * Update the keyboard and modifier mapping.
 * We need the KeyboardMapping for AddKeysym.
 * Modifier mapping is required for clearing the modifiers.
 */
void AutoTypePlatformX11::updateKeymap()
{
    if (m_xkb) {
        XkbFreeKeyboard(m_xkb, XkbAllComponentsMask, True);
    }
    m_xkb = getKeyboard();

    XDisplayKeycodes(m_dpy, &m_minKeycode, &m_maxKeycode);
    if (m_keysymTable != nullptr) {
        XFree(m_keysymTable);
    }
    m_keysymTable = XGetKeyboardMapping(m_dpy, m_minKeycode, m_maxKeycode - m_minKeycode + 1, &m_keysymPerKeycode);

    /* determine the keycode to use for remapped keys */
    if (m_remapKeycode == 0 || !isRemapKeycodeValid()) {
        for (int keycode = m_minKeycode; keycode <= m_maxKeycode; keycode++) {
            int inx = (keycode - m_minKeycode) * m_keysymPerKeycode;
            if (m_keysymTable[inx] == NoSymbol) {
                m_remapKeycode = keycode;
                m_currentRemapKeysym = NoSymbol;
                break;
            }
        }
    }

    /* determine the keycode to use for modifiers */
    XModifierKeymap* modifiers = XGetModifierMapping(m_dpy);
    for (int mod_index = ShiftMapIndex; mod_index <= Mod5MapIndex; mod_index++) {
        m_modifier_keycode[mod_index] = 0;
        for (int mod_key = 0; mod_key < modifiers->max_keypermod; mod_key++) {
            int keycode = modifiers->modifiermap[mod_index * modifiers->max_keypermod + mod_key];
            if (keycode) {
                m_modifier_keycode[mod_index] = keycode;
                break;
            }
        }
    }
    XFreeModifiermap(modifiers);

    /* Xlib needs some time until the mapping is distributed to
       all clients */
    Tools::sleep(30);
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
void AutoTypePlatformX11::sendKey(KeySym keysym, unsigned int modifiers)
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
    m_platform->sendKey(qcharToNativeKeyCode(action->character));
}

void AutoTypeExecutorX11::execKey(AutoTypeKey* action)
{
    m_platform->sendKey(qtToNativeKeyCode(action->key));
}

void AutoTypeExecutorX11::execClearField(AutoTypeClearField* action = nullptr)
{
    Q_UNUSED(action);

    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 25 * 1000 * 1000;

    m_platform->sendKey(qtToNativeKeyCode(Qt::Key_Home), static_cast<unsigned int>(ControlMask));
    nanosleep(&ts, nullptr);

    m_platform->sendKey(qtToNativeKeyCode(Qt::Key_End), static_cast<unsigned int>(ControlMask | ShiftMask));
    nanosleep(&ts, nullptr);

    m_platform->sendKey(qtToNativeKeyCode(Qt::Key_Backspace));
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
