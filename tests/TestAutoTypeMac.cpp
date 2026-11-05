/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "TestAutoTypeMac.h"

#include "autotype/mac/AutoTypeMac_p.h"

#include <CoreFoundation/CoreFoundation.h>
#include <QTest>

void TestAutoTypeMac::initTestCase()
{
    QLocale::setDefault(QLocale::c());
}

// Forward lookup: produce a glyph from a keycode + UCKeyTranslate modifier state
static bool
translateKeyCode(const UCKeyboardLayout* layout, UInt32 keyboardType, uint16_t keyCode, UInt32 mods, QChar& outGlyph)
{
    UInt32 deadKeyState = 0;
    UniCharCount actualLen = 0;
    UniChar unicode[4];

    OSStatus status = UCKeyTranslate(layout,
                                     keyCode,
                                     kUCKeyActionDown,
                                     mods,
                                     keyboardType,
                                     kUCKeyTranslateNoDeadKeysBit,
                                     &deadKeyState,
                                     4,
                                     &actualLen,
                                     unicode);
    if (status != noErr || actualLen != 1) {
        return false;
    }
    outGlyph = QChar(unicode[0]);
    return true;
}

// Load a specific keyboard layout by its input source ID (e.g. "com.apple.keylayout.US")
static const UCKeyboardLayout* layoutById(const char* sourceId)
{
    CFStringRef idRef = CFStringCreateWithCString(kCFAllocatorDefault, sourceId, kCFStringEncodingUTF8);
    CFStringRef key = kTISPropertyInputSourceID;
    CFDictionaryRef filter = CFDictionaryCreate(kCFAllocatorDefault,
                                                reinterpret_cast<const void**>(&key),
                                                reinterpret_cast<const void**>(&idRef),
                                                1,
                                                &kCFTypeDictionaryKeyCallBacks,
                                                &kCFTypeDictionaryValueCallBacks);
    CFArrayRef sources = TISCreateInputSourceList(filter, true);
    CFRelease(idRef);
    CFRelease(filter);
    if (!sources || CFArrayGetCount(sources) == 0) {
        if (sources)
            CFRelease(sources);
        return nullptr;
    }

    TISInputSourceRef keyboard = static_cast<TISInputSourceRef>(const_cast<void*>(CFArrayGetValueAtIndex(sources, 0)));
    CFDataRef layoutData =
        static_cast<CFDataRef>(TISGetInputSourceProperty(keyboard, kTISPropertyUnicodeKeyLayoutData));
    const UCKeyboardLayout* layout = nullptr;
    if (layoutData) {
        layout = reinterpret_cast<const UCKeyboardLayout*>(CFDataGetBytePtr(layoutData));
    }
    CFRelease(sources);
    return layout;
}

void TestAutoTypeMac::testReverseLookupRoundTrip()
{
    auto layout = layoutById("com.apple.keylayout.US");
    QVERIFY2(layout, "U.S. keyboard layout not available");
    UInt32 kbType = LMGetKbdType();

    static const UInt32 kUCTShift = shiftKey >> 8;
    static const UInt32 kUCTOption = optionKey >> 8;
    static const UInt32 mods[] = {0, kUCTShift, kUCTOption, kUCTShift | kUCTOption};

    for (uint16_t keyCode = 0; keyCode < 128; keyCode++) {
        for (UInt32 mod : mods) {
            QChar glyph;
            if (!translateKeyCode(layout, kbType, keyCode, mod, glyph)) {
                continue;
            }

            // Reverse-lookup the glyph
            uint16_t resolvedKeyCode = 0;
            CGEventFlags resolvedFlags = 0;
            QVERIFY2(charToNativeKeyCode(layout, kbType, glyph, resolvedKeyCode, resolvedFlags),
                     qPrintable(QString("charToNativeKeyCode returned false for '%1' (U+%2)")
                                    .arg(glyph)
                                    .arg(static_cast<int>(glyph.unicode()), 4, 16, QChar('0'))));

            // Feed the resolved keycode+flags back through UCKeyTranslate
            // and verify we get the same glyph
            UInt32 resolvedMods = 0;
            if (resolvedFlags & kCGEventFlagMaskShift) {
                resolvedMods |= kUCTShift;
            }
            if (resolvedFlags & kCGEventFlagMaskAlternate) {
                resolvedMods |= kUCTOption;
            }

            QChar roundTripGlyph;
            QVERIFY2(translateKeyCode(layout, kbType, resolvedKeyCode, resolvedMods, roundTripGlyph),
                     qPrintable(QString("Round-trip UCKeyTranslate failed for '%1'").arg(glyph)));
            QCOMPARE(roundTripGlyph, glyph);
        }
    }
}

void TestAutoTypeMac::testUnmappableReturnsFalse()
{
    auto layout = layoutById("com.apple.keylayout.US");
    QVERIFY2(layout, "U.S. keyboard layout not available");
    UInt32 kbType = LMGetKbdType();

    uint16_t keyCode = 0;
    CGEventFlags flags = 0;
    // U+4E2D (中, CJK ideograph) — not a single keystroke on U.S. layout
    QVERIFY(!charToNativeKeyCode(layout, kbType, QChar(0x4E2D), keyCode, flags));
}

void TestAutoTypeMac::testModifierMapping()
{
    auto layout = layoutById("com.apple.keylayout.US");
    QVERIFY2(layout, "U.S. keyboard layout not available");
    UInt32 kbType = LMGetKbdType();

    // 'A' (uppercase) should require shift
    uint16_t keyCode = 0;
    CGEventFlags flags = 0;
    QVERIFY(charToNativeKeyCode(layout, kbType, QChar('A'), keyCode, flags));
    QVERIFY2(flags & kCGEventFlagMaskShift,
             qPrintable(QString("Expected Shift flag for 'A', got flags=0x%1").arg(flags, 0, 16)));

    // 'a' (lowercase) should not require shift
    flags = 0;
    QVERIFY(charToNativeKeyCode(layout, kbType, QChar('a'), keyCode, flags));
    QVERIFY2(!(flags & kCGEventFlagMaskShift),
             qPrintable(QString("Expected no Shift flag for 'a', got flags=0x%1").arg(flags, 0, 16)));
}

QTEST_GUILESS_MAIN(TestAutoTypeMac)
