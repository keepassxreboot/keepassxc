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

#include <ApplicationServices/ApplicationServices.h>

#define HOTKEY_ID 1
#define MAX_WINDOW_TITLE_LENGTH 1024
#define INVALID_KEYCODE 0xFFFF

AutoTypePlatformMac::AutoTypePlatformMac()
    : m_appkit(new AppKit())
    , m_hotkeyRef(nullptr)
    , m_hotkeyId({ 'kpx2', HOTKEY_ID })
{
    EventTypeSpec eventSpec;
    eventSpec.eventClass = kEventClassKeyboard;
    eventSpec.eventKind = kEventHotKeyPressed;

    ::InstallApplicationEventHandler(AutoTypePlatformMac::hotkeyHandler, 1, &eventSpec, this, nullptr);
}

//
// Keepassx requires mac os 10.7
//
bool AutoTypePlatformMac::isAvailable()
{
    return true;
}

//
// Get list of visible window titles
// see: Quartz Window Services
//
QStringList AutoTypePlatformMac::windowTitles()
{
    QStringList list;

    CFArrayRef windowList = ::CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (windowList != nullptr) {
        CFIndex count = ::CFArrayGetCount(windowList);

        for (CFIndex i = 0; i < count; i++) {
            CFDictionaryRef window = static_cast<CFDictionaryRef>(::CFArrayGetValueAtIndex(windowList, i));
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

//
// Get active window process id
//
WId AutoTypePlatformMac::activeWindow()
{
    return m_appkit->activeProcessId();
}

//
// Get active window title
// see: Quartz Window Services
//
QString AutoTypePlatformMac::activeWindowTitle()
{
    QString title;

    CFArrayRef windowList = ::CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (windowList != nullptr) {
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

//
// Register global hotkey
//
bool AutoTypePlatformMac::registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    uint16 nativeKeyCode = qtToNativeKeyCode(key);
    if (nativeKeyCode == INVALID_KEYCODE) {
        qWarning("Invalid key code");
        return false;
    }
    CGEventFlags nativeModifiers = qtToNativeModifiers(modifiers, false);
    if (::RegisterEventHotKey(nativeKeyCode, nativeModifiers, m_hotkeyId, GetApplicationEventTarget(), 0, &m_hotkeyRef) != noErr) {
        qWarning("Register hotkey failed");
        return false;
    }

    return true;
}

//
// Unregister global hotkey
//
void AutoTypePlatformMac::unregisterGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(key);
    Q_UNUSED(modifiers);

    ::UnregisterEventHotKey(m_hotkeyRef);
}

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

//
// Activate window by process id
//
bool AutoTypePlatformMac::raiseWindow(WId pid)
{
    return m_appkit->activateProcess(pid);
}

//
// Activate last active window
//
bool AutoTypePlatformMac::raiseLastActiveWindow()
{
    return m_appkit->activateProcess(m_appkit->lastActiveProcessId());
}

//
// Activate keepassx window
//
bool AutoTypePlatformMac::raiseOwnWindow()
{
    return m_appkit->activateProcess(m_appkit->ownProcessId());
}

//
// Send unicode character to active window
// see: Quartz Event Services
//
void AutoTypePlatformMac::sendChar(const QChar& ch, bool isKeyDown)
{
    CGEventRef keyEvent = ::CGEventCreateKeyboardEvent(nullptr, 0, isKeyDown);
    if (keyEvent != nullptr) {
        UniChar unicode = ch.unicode();
        ::CGEventKeyboardSetUnicodeString(keyEvent, 1, &unicode);
        ::CGEventPost(kCGSessionEventTap, keyEvent);
        ::CFRelease(keyEvent);
    }
}

//
// Send key code to active window
// see: Quartz Event Services
//
void AutoTypePlatformMac::sendKey(Qt::Key key, bool isKeyDown, Qt::KeyboardModifiers modifiers = 0)
{
    uint16 keyCode = qtToNativeKeyCode(key);
    if (keyCode == INVALID_KEYCODE) {
        return;
    }

    CGEventRef keyEvent = ::CGEventCreateKeyboardEvent(nullptr, keyCode, isKeyDown);
    CGEventFlags nativeModifiers = qtToNativeModifiers(modifiers, true);
    if (keyEvent != nullptr) {
        ::CGEventSetFlags(keyEvent, nativeModifiers);
        ::CGEventPost(kCGSessionEventTap, keyEvent);
        ::CFRelease(keyEvent);
    }
}

//
// Translate qt key code to mac os key code
// see: HIToolbox/Events.h
//
uint16 AutoTypePlatformMac::qtToNativeKeyCode(Qt::Key key)
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
            Q_ASSERT(false);
            return INVALID_KEYCODE;
    }
}

//
// Translate qt key modifiers to mac os modifiers
// see: https://doc.qt.io/qt-5/osx-issues.html#special-keys
//
CGEventFlags AutoTypePlatformMac::qtToNativeModifiers(Qt::KeyboardModifiers modifiers, bool native)
{
    CGEventFlags nativeModifiers = CGEventFlags(0);

    CGEventFlags shiftMod = CGEventFlags(shiftKey);
    CGEventFlags cmdMod = CGEventFlags(cmdKey);
    CGEventFlags optionMod = CGEventFlags(optionKey);
    CGEventFlags controlMod = CGEventFlags(controlKey);

    if (native) {
        shiftMod = kCGEventFlagMaskShift;
        cmdMod = kCGEventFlagMaskCommand;
        optionMod = kCGEventFlagMaskAlternate;
        controlMod = kCGEventFlagMaskControl;
    }


    if (modifiers & Qt::ShiftModifier) {
        nativeModifiers = CGEventFlags(nativeModifiers | shiftMod);
    }
    if (modifiers & Qt::ControlModifier) {
        nativeModifiers = CGEventFlags(nativeModifiers | cmdMod);
    }
    if (modifiers & Qt::AltModifier) {
        nativeModifiers = CGEventFlags(nativeModifiers | optionMod);
    }
    if (modifiers & Qt::MetaModifier) {
        nativeModifiers = CGEventFlags(nativeModifiers | controlMod);
    }

    return nativeModifiers;
}

//
// Get window layer/level
//
int AutoTypePlatformMac::windowLayer(CFDictionaryRef window)
{
    int layer;

    CFNumberRef layerRef = static_cast<CFNumberRef>(::CFDictionaryGetValue(window, kCGWindowLayer));
    if (layerRef != nullptr
            && ::CFNumberGetValue(layerRef, kCFNumberIntType, &layer)) {
        return layer;
    }

    return -1;
}

//
// Get window title
//
QString AutoTypePlatformMac::windowTitle(CFDictionaryRef window)
{
    char buffer[MAX_WINDOW_TITLE_LENGTH];
    QString title;

    CFStringRef titleRef = static_cast<CFStringRef>(::CFDictionaryGetValue(window, kCGWindowName));
    if (titleRef != nullptr
            && ::CFStringGetCString(titleRef, buffer, MAX_WINDOW_TITLE_LENGTH, kCFStringEncodingUTF8)) {
        title = QString::fromUtf8(buffer);
    }

    return title;
}

//
// Carbon hotkey handler
//
OSStatus AutoTypePlatformMac::hotkeyHandler(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData)
{
    Q_UNUSED(nextHandler);

    AutoTypePlatformMac *self = static_cast<AutoTypePlatformMac *>(userData);
    EventHotKeyID hotkeyId;

    if (::GetEventParameter(theEvent, kEventParamDirectObject, typeEventHotKeyID, nullptr, sizeof(hotkeyId), nullptr, &hotkeyId) == noErr
            && hotkeyId.id == HOTKEY_ID) {
        emit self->globalShortcutTriggered();
    }

    return noErr;
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

void AutoTypeExecutorMac::execClearField(AutoTypeClearField* action = nullptr)
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
