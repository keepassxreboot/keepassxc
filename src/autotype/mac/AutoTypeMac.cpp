/*
 *  Copyright (C) 2016 Lennart Glauer <mail@lennart-glauer.de>
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

#include "AutoTypeMac.h"
#include "AutoTypeMacKeyCodes.h"
#include "gui/macutils/MacUtils.h"

#define MAX_WINDOW_TITLE_LENGTH 1024
#define INVALID_KEYCODE 0xFFFF

AutoTypePlatformMac::AutoTypePlatformMac()
    : m_globalMonitor(nullptr)
{
}

/**
 * Request accessibility permissions required for keyboard control
 *
 * @return true on success
 */
bool AutoTypePlatformMac::isAvailable()
{
    return macUtils()->enableAccessibility();
}

/**
 * Get a list of the currently open window titles
 *
 * @return list of window titles
 */
QStringList AutoTypePlatformMac::windowTitles()
{
    QStringList list;

    auto windowList = ::CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (windowList) {
        auto count = ::CFArrayGetCount(windowList);

        for (CFIndex i = 0; i < count; i++) {
            auto window = static_cast<CFDictionaryRef>(::CFArrayGetValueAtIndex(windowList, i));
            if (windowLayer(window) != 0) {
                continue;
            }

            QString title = windowTitle(window);
            if (!title.isEmpty()) {
                list.append(title);
            }
        }

        ::CFRelease(windowList);
    }

    return list;
}

/**
 * Get active window ID
 *
 * @return window ID
 */
WId AutoTypePlatformMac::activeWindow()
{
    return macUtils()->activeWindow();
}

/**
 * Get active window title
 *
 * @return window title
 */
QString AutoTypePlatformMac::activeWindowTitle()
{
    QString title;

    CFArrayRef windowList = ::CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (windowList) {
        CFIndex count = ::CFArrayGetCount(windowList);

        for (CFIndex i = 0; i < count; i++) {
            CFDictionaryRef window = static_cast<CFDictionaryRef>(::CFArrayGetValueAtIndex(windowList, i));
            if (windowLayer(window) == 0) {
                // First toplevel window in list (front to back order)
                title = windowTitle(window);
                if (!title.isEmpty()) {
                    break;
                }
            }
        }

        ::CFRelease(windowList);
    }

    return title;
}

/**
 * Register global hotkey using NS global event monitor.
 * Note that this hotkey is not trapped and may trigger
 * actions in the local application where it is issued.
 *
 * @param key key used for hotkey
 * @param modifiers modifiers required in addition to key
 * @return true on success
 */
bool AutoTypePlatformMac::registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    auto nativeKeyCode = qtToNativeKeyCode(key);
    if (nativeKeyCode == INVALID_KEYCODE) {
        qWarning("Invalid key code");
        return false;
    }
    auto nativeModifiers = qtToNativeModifiers(modifiers);
    m_globalMonitor =
        macUtils()->addGlobalMonitor(nativeKeyCode, nativeModifiers, this, AutoTypePlatformMac::hotkeyHandler);
    return true;
}

/**
 * Handle global hotkey presses by emitting the trigger signal
 *
 * @param userData pointer to AutoTypePlatform
 */
void AutoTypePlatformMac::hotkeyHandler(void* userData)
{
    auto* self = static_cast<AutoTypePlatformMac*>(userData);
    emit self->globalShortcutTriggered();
}

/**
 * Unregister a previously registered global hotkey
 *
 * @param key unused
 * @param modifiers unused
 */
void AutoTypePlatformMac::unregisterGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(key);
    Q_UNUSED(modifiers);
    if (m_globalMonitor) {
        macUtils()->removeGlobalMonitor(m_globalMonitor);
        m_globalMonitor = nullptr;
    }
}

/**
 * Unused
 */
int AutoTypePlatformMac::platformEventFilter(void* event)
{
    Q_UNUSED(event);
    Q_ASSERT(false);
    return -1;
}

AutoTypeExecutor* AutoTypePlatformMac::createExecutor()
{
    return new AutoTypeExecutorMac(this);
}

/**
 * Raise the given window ID
 *
 * @return true on success
 */
bool AutoTypePlatformMac::raiseWindow(WId pid)
{
    return macUtils()->raiseWindow(pid);
}

/**
 * Hide the KeePassXC window
 *
 * @return true on success
 */
bool AutoTypePlatformMac::hideOwnWindow()
{
    return macUtils()->hideOwnWindow();
}

//
// Activate keepassx window
//
bool AutoTypePlatformMac::raiseOwnWindow()
{
    return macUtils()->raiseOwnWindow();
}

/**
 * Send provided character as key event to the active window
 *
 * @param ch unicode character
 * @param isKeyDown whether the key is pressed
 */
void AutoTypePlatformMac::sendChar(const QChar& ch, bool isKeyDown)
{
    auto keyEvent = ::CGEventCreateKeyboardEvent(nullptr, 0, isKeyDown);
    if (keyEvent) {
        auto unicode = ch.unicode();
        ::CGEventKeyboardSetUnicodeString(keyEvent, 1, &unicode);
        ::CGEventPost(kCGSessionEventTap, keyEvent);
        ::CFRelease(keyEvent);
    }
}

/**
 * Send provided Qt key as key event to the active window
 *
 * @param key Qt key code
 * @param isKeyDown whether the key is pressed
 * @param modifiers any modifiers to apply to key
 */
void AutoTypePlatformMac::sendKey(Qt::Key key, bool isKeyDown, Qt::KeyboardModifiers modifiers)
{
    auto keyCode = qtToNativeKeyCode(key);
    if (keyCode == INVALID_KEYCODE) {
        return;
    }

    auto keyEvent = ::CGEventCreateKeyboardEvent(nullptr, keyCode, isKeyDown);
    auto nativeModifiers = qtToNativeModifiers(modifiers);
    if (keyEvent) {
        ::CGEventSetFlags(keyEvent, nativeModifiers);
        ::CGEventPost(kCGSessionEventTap, keyEvent);
        ::CFRelease(keyEvent);
    }
}

/**
 * Translate Qt key to macOS key code provided by
 * AutoTypeMacKeyCodes.h which are derived from
 * legacy Carbon "HIToolbox/Events.h"
 *
 * @param key key to translate
 * @returns macOS key code
 */
CGKeyCode AutoTypePlatformMac::qtToNativeKeyCode(Qt::Key key)
{
    switch (key) {
    case Qt::Key_A:
        return kVK_ANSI_A;
    case Qt::Key_B:
        return kVK_ANSI_B;
    case Qt::Key_C:
        return kVK_ANSI_C;
    case Qt::Key_D:
        return kVK_ANSI_D;
    case Qt::Key_E:
        return kVK_ANSI_E;
    case Qt::Key_F:
        return kVK_ANSI_F;
    case Qt::Key_G:
        return kVK_ANSI_G;
    case Qt::Key_H:
        return kVK_ANSI_H;
    case Qt::Key_I:
        return kVK_ANSI_I;
    case Qt::Key_J:
        return kVK_ANSI_J;
    case Qt::Key_K:
        return kVK_ANSI_K;
    case Qt::Key_L:
        return kVK_ANSI_L;
    case Qt::Key_M:
        return kVK_ANSI_M;
    case Qt::Key_N:
        return kVK_ANSI_N;
    case Qt::Key_O:
        return kVK_ANSI_O;
    case Qt::Key_P:
        return kVK_ANSI_P;
    case Qt::Key_Q:
        return kVK_ANSI_Q;
    case Qt::Key_R:
        return kVK_ANSI_R;
    case Qt::Key_S:
        return kVK_ANSI_S;
    case Qt::Key_T:
        return kVK_ANSI_T;
    case Qt::Key_U:
        return kVK_ANSI_U;
    case Qt::Key_V:
        return kVK_ANSI_V;
    case Qt::Key_W:
        return kVK_ANSI_W;
    case Qt::Key_X:
        return kVK_ANSI_X;
    case Qt::Key_Y:
        return kVK_ANSI_Y;
    case Qt::Key_Z:
        return kVK_ANSI_Z;

    case Qt::Key_0:
        return kVK_ANSI_0;
    case Qt::Key_1:
        return kVK_ANSI_1;
    case Qt::Key_2:
        return kVK_ANSI_2;
    case Qt::Key_3:
        return kVK_ANSI_3;
    case Qt::Key_4:
        return kVK_ANSI_4;
    case Qt::Key_5:
        return kVK_ANSI_5;
    case Qt::Key_6:
        return kVK_ANSI_6;
    case Qt::Key_7:
        return kVK_ANSI_7;
    case Qt::Key_8:
        return kVK_ANSI_8;
    case Qt::Key_9:
        return kVK_ANSI_9;

    case Qt::Key_Equal:
        return kVK_ANSI_Equal;
    case Qt::Key_Minus:
        return kVK_ANSI_Minus;
    case Qt::Key_BracketRight:
        return kVK_ANSI_RightBracket;
    case Qt::Key_BracketLeft:
        return kVK_ANSI_LeftBracket;
    case Qt::Key_QuoteDbl:
        return kVK_ANSI_Quote;
    case Qt::Key_Semicolon:
        return kVK_ANSI_Semicolon;
    case Qt::Key_Backslash:
        return kVK_ANSI_Backslash;
    case Qt::Key_Comma:
        return kVK_ANSI_Comma;
    case Qt::Key_Slash:
        return kVK_ANSI_Slash;
    case Qt::Key_Period:
        return kVK_ANSI_Period;

    case Qt::Key_Shift:
        return kVK_Shift;
    case Qt::Key_Control:
        return kVK_Command;
    case Qt::Key_Backspace:
        return kVK_Delete;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        return kVK_Tab;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        return kVK_Return;
    case Qt::Key_CapsLock:
        return kVK_CapsLock;
    case Qt::Key_Escape:
        return kVK_Escape;
    case Qt::Key_Space:
        return kVK_Space;
    case Qt::Key_PageUp:
        return kVK_PageUp;
    case Qt::Key_PageDown:
        return kVK_PageDown;
    case Qt::Key_End:
        return kVK_End;
    case Qt::Key_Home:
        return kVK_Home;
    case Qt::Key_Left:
        return kVK_LeftArrow;
    case Qt::Key_Up:
        return kVK_UpArrow;
    case Qt::Key_Right:
        return kVK_RightArrow;
    case Qt::Key_Down:
        return kVK_DownArrow;
    case Qt::Key_Delete:
        return kVK_ForwardDelete;
    case Qt::Key_Help:
        return kVK_Help;

    case Qt::Key_F1:
        return kVK_F1;
    case Qt::Key_F2:
        return kVK_F2;
    case Qt::Key_F3:
        return kVK_F3;
    case Qt::Key_F4:
        return kVK_F4;
    case Qt::Key_F5:
        return kVK_F5;
    case Qt::Key_F6:
        return kVK_F6;
    case Qt::Key_F7:
        return kVK_F7;
    case Qt::Key_F8:
        return kVK_F8;
    case Qt::Key_F9:
        return kVK_F9;
    case Qt::Key_F10:
        return kVK_F10;
    case Qt::Key_F11:
        return kVK_F11;
    case Qt::Key_F12:
        return kVK_F12;
    case Qt::Key_F13:
        return kVK_F13;
    case Qt::Key_F14:
        return kVK_F14;
    case Qt::Key_F15:
        return kVK_F15;
    case Qt::Key_F16:
        return kVK_F16;

    default:
        return INVALID_KEYCODE;
    }
}

/**
 * Translate Qt key modifiers to macOS key modifiers
 * provided by the Core Graphics component
 *
 * @param modifiers Qt keyboard modifier(s)
 * @returns macOS key modifier(s)
 */
CGEventFlags AutoTypePlatformMac::qtToNativeModifiers(Qt::KeyboardModifiers modifiers)
{
    auto nativeModifiers = CGEventFlags(0);

    if (modifiers & Qt::ShiftModifier) {
        nativeModifiers = CGEventFlags(nativeModifiers | kCGEventFlagMaskShift);
    }
    if (modifiers & Qt::ControlModifier) {
        nativeModifiers = CGEventFlags(nativeModifiers | kCGEventFlagMaskCommand);
    }
    if (modifiers & Qt::AltModifier) {
        nativeModifiers = CGEventFlags(nativeModifiers | kCGEventFlagMaskAlternate);
    }
    if (modifiers & Qt::MetaModifier) {
        nativeModifiers = CGEventFlags(nativeModifiers | kCGEventFlagMaskControl);
    }

    return nativeModifiers;
}

/**
 * Get window layer / level
 *
 * @param window macOS window ref
 * @returns layer number or -1 if window not found
 */
int AutoTypePlatformMac::windowLayer(CFDictionaryRef window)
{
    int layer;

    auto layerRef = static_cast<CFNumberRef>(::CFDictionaryGetValue(window, kCGWindowLayer));
    if (layerRef && ::CFNumberGetValue(layerRef, kCFNumberIntType, &layer)) {
        return layer;
    }

    return -1;
}

/**
 * Get window title for macOS window ref
 *
 * @param window macOS window ref
 * @returns window title if found
 */
QString AutoTypePlatformMac::windowTitle(CFDictionaryRef window)
{
    char buffer[MAX_WINDOW_TITLE_LENGTH];
    QString title;

    auto titleRef = static_cast<CFStringRef>(::CFDictionaryGetValue(window, kCGWindowName));
    if (titleRef && ::CFStringGetCString(titleRef, buffer, MAX_WINDOW_TITLE_LENGTH, kCFStringEncodingUTF8)) {
        title = QString::fromUtf8(buffer);
    }

    return title;
}

//
// ------------------------------ AutoTypeExecutorMac ------------------------------
//

AutoTypeExecutorMac::AutoTypeExecutorMac(AutoTypePlatformMac* platform)
    : m_platform(platform)
{
}

void AutoTypeExecutorMac::execChar(AutoTypeChar* action)
{
    m_platform->sendChar(action->character, true);
    m_platform->sendChar(action->character, false);
}

void AutoTypeExecutorMac::execKey(AutoTypeKey* action)
{
    m_platform->sendKey(action->key, true);
    m_platform->sendKey(action->key, false);
}

void AutoTypeExecutorMac::execClearField(AutoTypeClearField* action)
{
    Q_UNUSED(action);

    m_platform->sendKey(Qt::Key_Control, true, Qt::ControlModifier);
    m_platform->sendKey(Qt::Key_Up, true, Qt::ControlModifier);
    m_platform->sendKey(Qt::Key_Up, false, Qt::ControlModifier);
    m_platform->sendKey(Qt::Key_Control, false);
    usleep(25 * 1000);
    m_platform->sendKey(Qt::Key_Shift, true, Qt::ShiftModifier);
    m_platform->sendKey(Qt::Key_Control, true, Qt::ShiftModifier | Qt::ControlModifier);
    m_platform->sendKey(Qt::Key_Down, true, Qt::ShiftModifier | Qt::ControlModifier);
    m_platform->sendKey(Qt::Key_Down, false, Qt::ShiftModifier | Qt::ControlModifier);
    m_platform->sendKey(Qt::Key_Control, false, Qt::ShiftModifier);
    m_platform->sendKey(Qt::Key_Shift, false);
    usleep(25 * 1000);
    m_platform->sendKey(Qt::Key_Backspace, true);
    m_platform->sendKey(Qt::Key_Backspace, false);

    usleep(25 * 1000);
}
