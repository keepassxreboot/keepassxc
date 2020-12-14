/*
 * Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "X11Funcs.h"

#include "KeySymMap.h"
#include "core/Tools.h"

#include <X11/Xutil.h>

KeySym qcharToNativeKeyCode(const QChar& ch)
{
    ushort unicode = ch.unicode();

    /* first check for Latin-1 characters (1:1 mapping) */
    if ((unicode >= 0x0020 && unicode <= 0x007e) || (unicode >= 0x00a0 && unicode <= 0x00ff)) {
        return unicode;
    }

    /* mapping table generated from keysymdef.h */
    const uint* match = Tools::binaryFind(unicodeToKeysymKeys, unicodeToKeysymKeys + unicodeToKeysymLen, unicode);
    int index = match - unicodeToKeysymKeys;
    if (index != unicodeToKeysymLen) {
        return unicodeToKeysymValues[index];
    }

    if (unicode >= 0x0100) {
        return unicode | 0x01000000;
    }

    return NoSymbol;
}

KeySym qtToNativeKeyCode(Qt::Key key)
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

uint qtToNativeModifiers(Qt::KeyboardModifiers modifiers)
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
