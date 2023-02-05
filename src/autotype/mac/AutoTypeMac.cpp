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
#include "core/Tools.h"
#include "gui/osutils/macutils/MacUtils.h"
#include "gui/MessageBox.h"

#include <ApplicationServices/ApplicationServices.h>

#define INVALID_KEYCODE 0xFFFF

AutoTypePlatformMac::AutoTypePlatformMac()
{
    MessageBox::initializeButtonDefs();
}

/**
 * Determine if Auto-Type is available
 *
 * @return always return true
 */
bool AutoTypePlatformMac::isAvailable()
{
    // Accessibility permissions are requested upon first use, instead of on load
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

            QString title = windowStringProperty(window, kCGWindowName);
            QString owner = windowStringProperty(window, kCGWindowOwnerName);

            // Audio recording injects a "StatusIndicator" window owned by the "Window Server" process
            // into to list in macOS 12.2 (see: https://github.com/keepassxreboot/keepassxc/issues/7418).
            if (title == "StatusIndicator" && owner == "Window Server") {
                continue;
            }

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
    return macUtils()->activeWindow();
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
                title = windowStringProperty(window, kCGWindowName);
                QString owner = windowStringProperty(window, kCGWindowOwnerName);

                // Audio recording injects a "StatusIndicator" window owned by the "Window Server" process
                // into to list in macOS 12.2 (see: https://github.com/keepassxreboot/keepassxc/issues/7418).
                if (title == "StatusIndicator" && owner == "Window Server") {
                    continue;
                }

                // First toplevel window in list (front to back order)
                if (!title.isEmpty()) {
                    break;
                }
            }
        }

        ::CFRelease(windowList);
    }

    return title;
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
    return macUtils()->raiseWindow(pid);
}

//
// Activate last active window
//
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
void AutoTypePlatformMac::sendKey(Qt::Key key, bool isKeyDown, Qt::KeyboardModifiers modifiers)
{
    uint16 keyCode = macUtils()->qtToNativeKeyCode(key);
    if (keyCode == INVALID_KEYCODE) {
        return;
    }

    CGEventRef keyEvent = ::CGEventCreateKeyboardEvent(nullptr, keyCode, isKeyDown);
    CGEventFlags nativeModifiers = macUtils()->qtToNativeModifiers(modifiers, true);
    if (keyEvent != nullptr) {
        ::CGEventSetFlags(keyEvent, nativeModifiers);
        ::CGEventPost(kCGSessionEventTap, keyEvent);
        ::CFRelease(keyEvent);
    }
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
// Get window string property
//
QString AutoTypePlatformMac::windowStringProperty(CFDictionaryRef window, CFStringRef propertyRef)
{
    char buffer[1024];
    QString value;

    CFStringRef valueRef = static_cast<CFStringRef>(::CFDictionaryGetValue(window, propertyRef));
    if (valueRef != nullptr
            && ::CFStringGetCString(valueRef, buffer, 1024, kCFStringEncodingUTF8)) {
        value = QString::fromUtf8(buffer);
    }

    return value;
}

//
// ------------------------------ AutoTypeExecutorMac ------------------------------
//

AutoTypeExecutorMac::AutoTypeExecutorMac(AutoTypePlatformMac* platform)
    : m_platform(platform)
{
}

AutoTypeAction::Result AutoTypeExecutorMac::execBegin(const AutoTypeBegin* action)
{
    Q_UNUSED(action);
    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExecutorMac::execType(const AutoTypeKey* action)
{


    if (action->key != Qt::Key_unknown) {
        m_platform->sendKey(action->key, true, action->modifiers);
        m_platform->sendKey(action->key, false, action->modifiers);
    } else {
        if (action->modifiers != Qt::NoModifier) {
            // If we have modifiers set than we intend to send a key sequence
            // convert to uppercase to align with Qt Key mappings
            int ch = action->character.toUpper().toLatin1();
            m_platform->sendKey(static_cast<Qt::Key>(ch), true, action->modifiers);
            m_platform->sendKey(static_cast<Qt::Key>(ch), false, action->modifiers);
        } else if (mode == Mode::VIRTUAL) {
            int ch = action->character.toLatin1();
            m_platform->sendKey(static_cast<Qt::Key>(ch), true, action->modifiers);
            m_platform->sendKey(static_cast<Qt::Key>(ch), false, action->modifiers);
        } else {
            m_platform->sendChar(action->character, true);
            m_platform->sendChar(action->character, false);
        }
    }

    Tools::sleep(execDelayMs);
    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExecutorMac::execClearField(const AutoTypeClearField* action)
{
    Q_UNUSED(action);
    execType(new AutoTypeKey(Qt::Key_Left, Qt::ControlModifier));
    execType(new AutoTypeKey(Qt::Key_Right, Qt::ControlModifier | Qt::ShiftModifier));
    execType(new AutoTypeKey(Qt::Key_Backspace));
    return AutoTypeAction::Result::Ok();
}
