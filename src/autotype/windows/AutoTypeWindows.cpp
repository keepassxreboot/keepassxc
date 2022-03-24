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

#include "AutoTypeWindows.h"
#include "core/Tools.h"
#include "gui/osutils/OSUtils.h"

#include <VersionHelpers.h>

#define HOTKEY_ID 1
#define MAX_WINDOW_TITLE_LENGTH 1024

#define MOD_NOREPEAT 0x4000 // Missing in MinGW

//
// Test if os version is Windows 7 or later
//
bool AutoTypePlatformWin::isAvailable()
{
    return IsWindows7OrGreater();
}

//
// Get list of all visible window titles
//
QStringList AutoTypePlatformWin::windowTitles()
{
    QStringList list;

    ::EnumWindows(AutoTypePlatformWin::windowTitleEnumProc, reinterpret_cast<LPARAM>(&list));

    return list;
}

//
// Get foreground window hwnd
//
WId AutoTypePlatformWin::activeWindow()
{
    return reinterpret_cast<WId>(::GetForegroundWindow());
}

//
// Get foreground window title
//
QString AutoTypePlatformWin::activeWindowTitle()
{
    return windowTitle(::GetForegroundWindow());
}

AutoTypeExecutor* AutoTypePlatformWin::createExecutor()
{
    return new AutoTypeExecutorWin(this);
}

//
// Set foreground window
//
bool AutoTypePlatformWin::raiseWindow(WId window)
{
    HWND hwnd = reinterpret_cast<HWND>(window);

    return ::BringWindowToTop(hwnd) && ::SetForegroundWindow(hwnd);
}

//
// Send unicode character to foreground window
//
void AutoTypePlatformWin::sendChar(const QChar& ch)
{
    INPUT in[2];
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = 0;
    in[0].ki.wScan = ch.unicode();
    in[0].ki.dwFlags = KEYEVENTF_UNICODE;
    in[0].ki.time = 0;
    in[0].ki.dwExtraInfo = ::GetMessageExtraInfo();

    in[1] = in[0];
    in[1].ki.dwFlags |= KEYEVENTF_KEYUP;

    ::SendInput(2, &in[0], sizeof(INPUT));
}

void AutoTypePlatformWin::sendCharVirtual(const QChar& ch)
{
    auto vKey = VkKeyScanExW(ch.unicode(), GetKeyboardLayout(0));
    if (vKey == -1) {
        // VKey not found, send as Unicode character
        sendChar(ch);
        return;
    }

    if (HIBYTE(vKey) & 0x6) {
        setKeyState(Qt::Key_AltGr, true);
    } else {
        if (HIBYTE(vKey) & 0x1) {
            setKeyState(Qt::Key_Shift, true);
        }
        if (HIBYTE(vKey) & 0x2) {
            setKeyState(Qt::Key_Control, true);
        }
        if (HIBYTE(vKey) & 0x4) {
            setKeyState(Qt::Key_Alt, true);
        }
    }

    INPUT in[2];
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = 0;
    in[0].ki.wScan = MapVirtualKey(LOBYTE(vKey), MAPVK_VK_TO_VSC);
    in[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    in[0].ki.time = 0;
    in[0].ki.dwExtraInfo = ::GetMessageExtraInfo();

    in[1] = in[0];
    in[1].ki.dwFlags |= KEYEVENTF_KEYUP;

    ::SendInput(2, &in[0], sizeof(INPUT));

    if (HIBYTE(vKey) & 0x6) {
        setKeyState(Qt::Key_AltGr, false);
    } else {
        if (HIBYTE(vKey) & 0x1) {
            setKeyState(Qt::Key_Shift, false);
        }
        if (HIBYTE(vKey) & 0x2) {
            setKeyState(Qt::Key_Control, false);
        }
        if (HIBYTE(vKey) & 0x4) {
            setKeyState(Qt::Key_Alt, false);
        }
    }
}

//
// Send virtual key code to foreground window
//
void AutoTypePlatformWin::setKeyState(Qt::Key key, bool down)
{
    WORD nativeKeyCode = winUtils()->qtToNativeKeyCode(key);
    DWORD nativeFlags = KEYEVENTF_SCANCODE;
    if (isExtendedKey(nativeKeyCode)) {
        nativeFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    if (!down) {
        nativeFlags |= KEYEVENTF_KEYUP;
    }

    INPUT in;
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = 0;
    in.ki.wScan = MapVirtualKey(LOBYTE(nativeKeyCode), MAPVK_VK_TO_VSC);
    in.ki.dwFlags = nativeFlags;
    in.ki.time = 0;
    in.ki.dwExtraInfo = ::GetMessageExtraInfo();

    ::SendInput(1, &in, sizeof(INPUT));
}

//
// The extended-key flag indicates whether the keystroke message originated
// from one of the additional keys on the enhanced keyboard
// see: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646267%28v=vs.85%29.aspx#EXTENDED_KEY_FLAG
//
bool AutoTypePlatformWin::isExtendedKey(DWORD nativeKeyCode)
{
    switch (nativeKeyCode) {
    case VK_RMENU:
    case VK_RCONTROL:
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_NUMLOCK:
    case VK_CANCEL:
    case VK_SNAPSHOT:
    case VK_DIVIDE:
    case VK_LWIN:
    case VK_RWIN:
    case VK_APPS:
        return true;
    default:
        return false;
    }
}
// clang-format on

//
// Test if window is in Alt+Tab list
// see: https://blogs.msdn.microsoft.com/oldnewthing/20071008-00/?p=24863
//
bool AutoTypePlatformWin::isAltTabWindow(HWND hwnd)
{
    if (!::IsWindowVisible(hwnd)) {
        return false;
    }

    // Start at the root owner
    HWND hwndWalk = ::GetAncestor(hwnd, GA_ROOTOWNER);
    HWND hwndTry;

    // See if we are the last active visible popup
    while ((hwndTry = ::GetLastActivePopup(hwndWalk)) != hwndWalk) {
        if (::IsWindowVisible(hwndTry)) {
            break;
        }
        hwndWalk = hwndTry;
    }

    return hwndWalk == hwnd;
}

//
// Window title enum proc
//
BOOL CALLBACK AutoTypePlatformWin::windowTitleEnumProc(_In_ HWND hwnd, _In_ LPARAM lParam)
{
    if (!isAltTabWindow(hwnd)) {
        // Skip window
        return TRUE;
    }

    QStringList* list = reinterpret_cast<QStringList*>(lParam);
    QString title = windowTitle(hwnd);

    if (!title.isEmpty()) {
        list->append(title);
    }

    return TRUE;
}

//
// Get window title
//
QString AutoTypePlatformWin::windowTitle(HWND hwnd)
{
    wchar_t title[MAX_WINDOW_TITLE_LENGTH];
    int count = ::GetWindowTextW(hwnd, title, MAX_WINDOW_TITLE_LENGTH);

    return QString::fromUtf16(reinterpret_cast<const ushort*>(title), count);
}

//
// ------------------------------ AutoTypeExecutorWin ------------------------------
//

AutoTypeExecutorWin::AutoTypeExecutorWin(AutoTypePlatformWin* platform)
    : m_platform(platform)
{
}

AutoTypeAction::Result AutoTypeExecutorWin::execBegin(const AutoTypeBegin* action)
{
    Q_UNUSED(action);
    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExecutorWin::execType(const AutoTypeKey* action)
{
    if (action->modifiers & Qt::ShiftModifier) {
        m_platform->setKeyState(Qt::Key_Shift, true);
    }
    if (action->modifiers & Qt::ControlModifier) {
        m_platform->setKeyState(Qt::Key_Control, true);
    }
    if (action->modifiers & Qt::AltModifier) {
        m_platform->setKeyState(Qt::Key_Alt, true);
    }
    if (action->modifiers & Qt::MetaModifier) {
        m_platform->setKeyState(Qt::Key_Meta, true);
    }

    if (action->key != Qt::Key_unknown) {
        m_platform->setKeyState(action->key, true);
        m_platform->setKeyState(action->key, false);
    } else {
        if (mode == Mode::VIRTUAL || action->modifiers != Qt::NoModifier) {
            m_platform->sendCharVirtual(action->character);
        } else {
            m_platform->sendChar(action->character);
        }
    }

    if (action->modifiers & Qt::ShiftModifier) {
        m_platform->setKeyState(Qt::Key_Shift, false);
    }
    if (action->modifiers & Qt::ControlModifier) {
        m_platform->setKeyState(Qt::Key_Control, false);
    }
    if (action->modifiers & Qt::AltModifier) {
        m_platform->setKeyState(Qt::Key_Alt, false);
    }
    if (action->modifiers & Qt::MetaModifier) {
        m_platform->setKeyState(Qt::Key_Meta, false);
    }

    Tools::sleep(execDelayMs);
    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExecutorWin::execClearField(const AutoTypeClearField* action)
{
    Q_UNUSED(action);
    execType(new AutoTypeKey(Qt::Key_Home, Qt::ControlModifier));
    execType(new AutoTypeKey(Qt::Key_End, Qt::ControlModifier | Qt::ShiftModifier));
    execType(new AutoTypeKey(Qt::Key_Backspace));
    return AutoTypeAction::Result::Ok();
}
