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
void AutoTypePlatformWin::sendChar(const QChar& ch, bool isKeyDown)
{
    DWORD nativeFlags = KEYEVENTF_UNICODE;
    if (!isKeyDown) {
        nativeFlags |= KEYEVENTF_KEYUP;
    }

    INPUT in;
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = 0;
    in.ki.wScan = ch.unicode();
    in.ki.dwFlags = nativeFlags;
    in.ki.time = 0;
    in.ki.dwExtraInfo = ::GetMessageExtraInfo();

    ::SendInput(1, &in, sizeof(INPUT));
}

//
// Send virtual key code to foreground window
//
void AutoTypePlatformWin::sendKey(Qt::Key key, bool isKeyDown)
{
    DWORD nativeKeyCode = winUtils()->qtToNativeKeyCode(key);
    if (nativeKeyCode < 1 || nativeKeyCode > 254) {
        return;
    }
    DWORD nativeFlags = 0;
    if (isExtendedKey(nativeKeyCode)) {
        nativeFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    if (!isKeyDown) {
        nativeFlags |= KEYEVENTF_KEYUP;
    }

    INPUT in;
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = LOWORD(nativeKeyCode);
    in.ki.wScan = LOWORD(::MapVirtualKeyW(nativeKeyCode, MAPVK_VK_TO_VSC));
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

void AutoTypeExecutorWin::execChar(AutoTypeChar* action)
{
    m_platform->sendChar(action->character, true);
    m_platform->sendChar(action->character, false);
}

void AutoTypeExecutorWin::execKey(AutoTypeKey* action)
{
    m_platform->sendKey(action->key, true);
    m_platform->sendKey(action->key, false);
}

void AutoTypeExecutorWin::execClearField(AutoTypeClearField* action = nullptr)
{
    Q_UNUSED(action);

    m_platform->sendKey(Qt::Key_Control, true);
    m_platform->sendKey(Qt::Key_Home, true);
    m_platform->sendKey(Qt::Key_Home, false);
    m_platform->sendKey(Qt::Key_Control, false);
    ::Sleep(25);
    m_platform->sendKey(Qt::Key_Control, true);
    m_platform->sendKey(Qt::Key_Shift, true);
    m_platform->sendKey(Qt::Key_End, true);
    m_platform->sendKey(Qt::Key_End, false);
    m_platform->sendKey(Qt::Key_Shift, false);
    m_platform->sendKey(Qt::Key_Control, false);
    ::Sleep(25);
    m_platform->sendKey(Qt::Key_Backspace, true);
    m_platform->sendKey(Qt::Key_Backspace, false);

    ::Sleep(25);
}
