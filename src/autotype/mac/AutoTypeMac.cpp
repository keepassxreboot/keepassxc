/*
 *  Copyright (C) 2009-2010 Jeff Gibbons
 *  Copyright (C) 2005-2008 by Tarek Saidi <tarek.saidi@arcor.de>
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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
#include <map>

static pid_t keepassxPID2;

#define UNICODE_BUFFER_SIZE 20
static UniChar      unicodeBuffer[UNICODE_BUFFER_SIZE];
static UniCharCount unicodePtr = 0;
// reusable events
static CGEventRef unicodeEvent = CGEventCreateKeyboardEvent(NULL, 0, true);
CGEventRef AutoTypePlatformMac::keyEvent =
        CGEventCreateKeyboardEvent(NULL, 0, true);

bool AutoTypePlatformMac::inHotKeyEvent = false;

static const KeycodeWithMods NoKeycodeWithMods =
        (KeycodeWithMods){ NoKeycode, 0 };

static uint orderedModifiers[] = {
    0,
    (  shiftKey                                  ) >> 8,
    (controlKey                                  ) >> 8,
    ( optionKey                                  ) >> 8,
    ( cmdKey                                     ) >> 8,
    (  shiftKey | controlKey                     ) >> 8,
    (  shiftKey | optionKey                      ) >> 8,
    (  shiftKey | cmdKey                         ) >> 8,
    (controlKey | optionKey                      ) >> 8,
    (controlKey | cmdKey                         ) >> 8,
    ( optionKey | cmdKey                         ) >> 8,
    (  shiftKey | controlKey | optionKey         ) >> 8,
    (  shiftKey | controlKey | cmdKey            ) >> 8,
    (  shiftKey | optionKey  | cmdKey            ) >> 8,
    (controlKey | optionKey  | cmdKey            ) >> 8,
    (  shiftKey | controlKey | optionKey | cmdKey) >> 8
};

static std::map<uint,KeycodeWithMods> unicodeToKeycodeWithModsMap;


AutoTypePlatformMac::AutoTypePlatformMac() : first(true), keepassxPID(0)
{
    globalKey = 0;
    globalMod = 0;
    inGlobalAutoType = false;

    // initialize hot key handling
    hotKeyRef = NULL;
    hotKeyID.signature = 'kpsx';
    hotKeyID.id = 1;
    EventTypeSpec eventType;
    eventType.eventClass = kEventClassKeyboard;
    eventType.eventKind = kEventHotKeyPressed;
    InstallApplicationEventHandler(&hotKeyHandler, 1, &eventType, this, NULL);
    AutoTypePlatformMac::initUnicodeToKeycodeWithModsMap();
}


QStringList AutoTypePlatformMac::windowTitles()
{
    pid_t pid;
    return getTargetWindowInfo(&pid, NULL);
}


WId AutoTypePlatformMac::activeWindow()
{
    WId mid;
    pid_t pid;
    getTargetWindowInfo(&pid, &mid);
    AutoTypePlatformMac::processToFront(pid);
    if (first)
        first = false;

    return mid;
}


QString AutoTypePlatformMac::activeWindowTitle()
{
    QStringList sl = getTargetWindowInfo(NULL, NULL);
    return sl[0];
}


bool AutoTypePlatformMac::registerGlobalShortcut(Qt::Key key,
        Qt::KeyboardModifiers modifiers)
{
    if (key == 0)
        return false;

    //KeycodeWithMods kc = keyToKeycodeWithMods(key);
    KeycodeWithMods kc = AutoTypePlatformMac::keysymToKeycodeWithMods2(static_cast<KeySym>(key));
    int code = kc.keycode;
    uint mod = qtToNativeModifiers(modifiers);

    if (code==globalKey && mod==globalMod)
        return true;

    // need to unregister old before registering new
    unregisterGlobalShortcut(key, modifiers);
    OSStatus status = RegisterEventHotKey(code, mod, hotKeyID,
        GetApplicationEventTarget(), 0, &hotKeyRef);

    if (noErr == status) {
        globalKey = code;
        globalMod = mod;
        return true;
    } else {
        qWarning("Error registering global shortcut: %d", status);
        RegisterEventHotKey(globalKey, globalMod, hotKeyID,
                            GetApplicationEventTarget(), 0, &hotKeyRef);
        return false;
    }
}


void AutoTypePlatformMac::unregisterGlobalShortcut(Qt::Key,
        Qt::KeyboardModifiers)
{
    globalKey = 0;
    globalMod = 0;
    UnregisterEventHotKey(hotKeyRef);
}


int AutoTypePlatformMac::platformEventFilter(void* event)
{
    return -1;
}


int AutoTypePlatformMac::initialTimeout()
{
    first = true;
    return 500;
}


AutoTypeExecutor* AutoTypePlatformMac::createExecutor()
{
    return new AutoTypeExecturorMac(this);
}


void AutoTypePlatformMac::sendUnicode(KeySym keysym)
{
    if (onlySendKeycodes) {
        KeycodeWithMods keycodeWithMods =
            AutoTypePlatformMac::keysymToKeycodeWithMods2(keysym);
        if (NoKeycode == keycodeWithMods.keycode) return;
        sendKeycode(keycodeWithMods);
        return;
    }
    unicodeBuffer[unicodePtr++] = keysym;
    if (UNICODE_BUFFER_SIZE == unicodePtr) flushUnicode();
}


void AutoTypePlatformMac::sendKeycode(KeycodeWithMods keycodeWithMods)
{
    flushUnicode();
    uint keycode = keycodeWithMods.keycode;
    uint    mods = keycodeWithMods.mods << 8;
    uint   flags = 0;
    if (0 != (  shiftKey & mods)) flags |= kCGEventFlagMaskShift;
    if (0 != (controlKey & mods)) flags |= kCGEventFlagMaskControl;
    if (0 != ( optionKey & mods)) flags |= kCGEventFlagMaskAlternate;
    if (0 != (      cmdKey & mods)) flags |= kCGEventFlagMaskCommand;
    CGEventSetIntegerValueField(keyEvent, kCGKeyboardEventKeycode, keycode);
    CGEventSetFlags(AutoTypePlatformMac::keyEvent, flags);
    keyDownUp(AutoTypePlatformMac::keyEvent);
    sleepKeyStrokeDelay();
}


KeycodeWithMods AutoTypePlatformMac::keyToKeycodeWithMods(Qt::Key key)
{
    KeycodeWithMods kc;
    kc.mods = 0;

    switch (key) {
    case Qt::Key_Tab:
        kc.keycode = kVK_Tab;
        break;
    case Qt::Key_Enter:
        kc.keycode = kVK_Return;
        break;
    case Qt::Key_Up:
        kc.keycode = kVK_UpArrow;
        break;
    case Qt::Key_Down:
        kc.keycode = kVK_DownArrow;
        break;
    case Qt::Key_Left:
        kc.keycode = kVK_LeftArrow;
        break;
    case Qt::Key_Right:
        kc.keycode = kVK_RightArrow;
        break;
    case Qt::Key_Insert:
        kc.keycode = kVK_Help;
        break;
    case Qt::Key_Delete:
        kc.keycode = kVK_ForwardDelete;
        break;
    case Qt::Key_Home:
        kc.keycode = kVK_Home;
        break;
    case Qt::Key_End:
        kc.keycode = kVK_End;
        break;
    case Qt::Key_PageUp:
        kc.keycode = kVK_PageUp;
        break;
    case Qt::Key_PageDown:
        kc.keycode = kVK_PageDown;
        break;
    case Qt::Key_Backspace:
        kc.keycode = kVK_Delete;
        break;
    //case Qt::Key_Pause:
    //    return XK_Break;
    case Qt::Key_CapsLock:
        kc.keycode = kVK_CapsLock;
        break;
    case Qt::Key_Escape:
        kc.keycode = kVK_Escape;
        break;
    case Qt::Key_Help:
        kc.keycode = kVK_Help;
        break;
    case Qt::Key_NumLock:
        kc.keycode = kVK_ANSI_KeypadClear;
        break;
    case Qt::Key_Print:
        kc.keycode = kVK_F13;
        break;
    //case Qt::Key_ScrollLock:
    //    return XK_Scroll_Lock;
    default:
        if (key >= Qt::Key_F1 && key <= Qt::Key_F16)
            kc.keycode = kVK_F1 + key - Qt::Key_F1;
        else
            kc.keycode = NoSymbol;
    }

    return kc;
}

// Private

QStringList AutoTypePlatformMac::getTargetWindowInfo(pid_t *pidPtr,
         WId *windowNumberPtr)
{
    QStringList titles;

    const int maxWindowNameSize = 512;
    char windowName[maxWindowNameSize];

    onlySendKeycodes = false;

    CFArrayRef windowInfo = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        0);
    CFIndex windowCount = CFArrayGetCount(windowInfo);

    for (CFIndex i = 0; i < windowCount; i++) {
        CFDictionaryRef window = static_cast<CFDictionaryRef>
            (CFArrayGetValueAtIndex(windowInfo, i));

        // only want windows in layer 0
        CFNumberRef windowLayerRef = static_cast<CFNumberRef>
            (CFDictionaryGetValue(window, kCGWindowLayer));

        int windowLayer = -1;
        CFNumberGetValue(windowLayerRef, kCFNumberIntType, &windowLayer);
        if (0 != windowLayer) continue;

        // get the pid owning this window
        CFNumberRef pidRef = static_cast<CFNumberRef>
            (CFDictionaryGetValue(window, kCGWindowOwnerPID));
        pid_t pid = -1;
        CFNumberGetValue(pidRef, kCFNumberIntType, &pid);

        // skip KeePassX windows
        if (getKeepassxPID() == pid) continue;

        // get window name; continue if no name
        CFStringRef windowNameRef = static_cast<CFStringRef>
            (CFDictionaryGetValue(window, kCGWindowName));
        if (!windowNameRef) continue;

        windowName[0] = 0;
        if (!CFStringGetCString(windowNameRef, windowName,
                                maxWindowNameSize, kCFStringEncodingUTF8) ||
            (0 == windowName[0])) continue;

        if (NULL != pidPtr)
            *pidPtr = pid;

        if (NULL != windowNumberPtr) {
            CGWindowID wid;
            CFNumberRef windowNumberRef = static_cast<CFNumberRef>
                (CFDictionaryGetValue(window, kCGWindowNumber));
            CFNumberGetValue(windowNumberRef, kCGWindowIDCFNumberType, &wid);
            *windowNumberPtr = wid;
            return titles;
        }
        titles.append(QString(windowName));
    }

    return titles;
}


uint AutoTypePlatformMac::qtToNativeModifiers(Qt::KeyboardModifiers modifiers)
{
    uint nativeModifiers = 0;

    if (modifiers & Qt::ShiftModifier) {
        nativeModifiers |= shiftKey;
    }
    if (modifiers & Qt::ControlModifier) {
        nativeModifiers |= controlKey;
    }
    if (modifiers & Qt::AltModifier) {
        nativeModifiers |= optionKey;
    }
    if (modifiers & Qt::MetaModifier) {
        nativeModifiers |= cmdKey;
    }

    return nativeModifiers;
}


void AutoTypePlatformMac::flushUnicode()
{
    if (0 == unicodePtr) return;
    CGEventKeyboardSetUnicodeString(unicodeEvent, unicodePtr, unicodeBuffer);
    keyDownUp(unicodeEvent);
    unicodePtr = 0;
    sleepKeyStrokeDelay();
}


void AutoTypePlatformMac::keyDownUp(CGEventRef theEvent)
{
    // posting Key Down/Up events also annoyingly sets mouse location so mus
    // current mouse location and set it in the event
    CGEventRef eventLocation = CGEventCreate(NULL);
    CGEventSetLocation(theEvent, CGEventGetLocation(eventLocation));
    CFRelease(eventLocation);

    CGEventSetType(theEvent, kCGEventKeyDown);
    CGEventPost(kCGHIDEventTap, theEvent);
    CGEventSetType(theEvent, kCGEventKeyUp);
    CGEventPost(kCGHIDEventTap, theEvent);
}


void AutoTypePlatformMac::sleepTime(int msec)
{
    if (msec == 0) return;

    timespec timeOut, remains;
    timeOut.tv_sec = msec/1000;
    timeOut.tv_nsec = (msec%1000)*1000000;
    nanosleep(&timeOut, &remains);
}


OSStatus AutoTypePlatformMac::hotKeyHandler(EventHandlerCallRef, EventRef,
                                            void *userData)
{
    // ignore nextHandler - should not be called
    if ((inHotKeyEvent) ||
        AutoTypePlatformMac::isFrontProcess(AutoTypePlatformMac::getKeepassxPID2())) return noErr;

    AutoTypePlatformMac::inHotKeyEvent = true;
    Q_EMIT static_cast<AutoTypePlatformMac*>(userData)->globalShortcutTriggered();
    AutoTypePlatformMac::inHotKeyEvent = false;
    return noErr;
}


pid_t AutoTypePlatformMac::getKeepassxPID() {

    if (0 == keepassxPID) {
        ProcessSerialNumber processSerialNumber;
        GetCurrentProcess(&processSerialNumber);
        GetProcessPID(&processSerialNumber, &keepassxPID);
    }

    return keepassxPID;
}


Boolean AutoTypePlatformMac::isFrontProcess(pid_t pid)
{
    Boolean result;
    ProcessSerialNumber pidPSN;
    ProcessSerialNumber frontPSN;
    OSStatus status = GetProcessForPID(pid, &pidPSN);
    if (noErr != status) {
        qWarning("AutoTypePlatformMac::isFrontProcess: GetProcessForPID "
                 "error for pid %d: %d", pid, status);
        return false;
    }
    GetFrontProcess(&frontPSN);
    SameProcess(&pidPSN, &frontPSN, &result);
    return result;
}


void AutoTypePlatformMac::initUnicodeToKeycodeWithModsMap()
{
    unicodeToKeycodeWithModsMap.clear();
    TISInputSourceRef inputSourceRef = TISCopyCurrentKeyboardInputSource();

    if (NULL == inputSourceRef) {
        qWarning("AutoTypePlatformMac::initUnicodeToKeycodeWithModsMap: "
                 "inputSourceRef is NULL");
        return;
    }

    CFDataRef unicodeKeyLayoutDataRef = static_cast<CFDataRef>
        (TISGetInputSourceProperty(inputSourceRef,
         kTISPropertyUnicodeKeyLayoutData));

    if (NULL == unicodeKeyLayoutDataRef) {
        qWarning("AutoTypePlatformMac::initUnicodeToKeycodeWithModsMap: "
                 "unicodeKeyLayoutDataRef is NULL");
        return;
    }
    const UCKeyboardLayout *unicodeKeyLayoutDataPtr =
        reinterpret_cast<const UCKeyboardLayout*>
            (CFDataGetBytePtr(unicodeKeyLayoutDataRef));

    UInt32 deadKeyState;
    UniChar unicodeString[8];
    UniCharCount len;
    for (int m = 0; m < 16; m++) {
        uint mods =  orderedModifiers[m];
        for (uint keycode = 0; keycode < 0x80; keycode++) {
            deadKeyState = 0;
            len = 0;
            OSStatus status = UCKeyTranslate(unicodeKeyLayoutDataPtr, keycode,
                kUCKeyActionDown, mods, LMGetKbdType(),
                kUCKeyTranslateNoDeadKeysMask, &deadKeyState,
                sizeof(unicodeString), &len, unicodeString);

            if (noErr != status) {
                qWarning("AutoTypePlatformMac::initUnicodeToKeycodeWithModsMap: "
                 "UCKeyTranslate error: %d keycode 0x%02X modifiers 0x%02X",
                         status, keycode, mods);
                continue;
            }

            // store if only one char and not already in store
            if ((1 != len) ||
                (0 < unicodeToKeycodeWithModsMap.count(unicodeString[0])))
                    continue;

            KeycodeWithMods kc;
            kc.keycode = keycode;
            kc.mods = mods;
            unicodeToKeycodeWithModsMap[unicodeString[0]] = kc;
        }
    }
}


void AutoTypePlatformMac::processToFront(pid_t pid)
{
    OSStatus status;
    ProcessSerialNumber processSerialNumber;
    status = GetProcessForPID(pid, &processSerialNumber);

    if (noErr != status) {
        qWarning("AutoTypePlatformMac::processToFront: GetProcessForPID "
                 "error for pid %d: %d", pid, status);
        return;
    }

    status = SetFrontProcessWithOptions(&processSerialNumber,
        kSetFrontProcessFrontWindowOnly);

    if (noErr != status) {
        qWarning("AutoTypePlatformMac::processToFront: "
                 "SetFrontProcessWithOptions for pid %d: %d", pid, status);
        return;
    }
}


KeycodeWithMods AutoTypePlatformMac::keysymToKeycodeWithMods2(KeySym keysym)
{
    return 0 == unicodeToKeycodeWithModsMap.count(keysym)
            ? NoKeycodeWithMods : unicodeToKeycodeWithModsMap[keysym];
}


pid_t AutoTypePlatformMac::getKeepassxPID2()
{
    if (0 == keepassxPID2) {
        ProcessSerialNumber processSerialNumber;
        GetCurrentProcess(&processSerialNumber);
        GetProcessPID(&processSerialNumber, &keepassxPID2);
    }
    return keepassxPID2;
}


AutoTypeExecturorMac::AutoTypeExecturorMac(AutoTypePlatformMac* platform)
    : m_platform(platform)
{
}


void AutoTypeExecturorMac::execChar(AutoTypeChar* action)
{
    m_platform->sendUnicode(action->character.unicode());
}


void AutoTypeExecturorMac::execKey(AutoTypeKey* action)
{
    m_platform->sendKeycode(m_platform->keyToKeycodeWithMods(action->key));
}


Q_EXPORT_PLUGIN2(keepassx-autotype-mac, AutoTypePlatformMac)
