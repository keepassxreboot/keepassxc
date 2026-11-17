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

#ifndef KEEPASSXC_AUTOTYPEMAC_P_H
#define KEEPASSXC_AUTOTYPEMAC_P_H

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <QChar>

/* Internal helper for reverse-looking a character to a native keycode. */

bool charToNativeKeyCode(const UCKeyboardLayout* layout,
                         UInt32 keyboardType,
                         const QChar& ch,
                         uint16_t& outKeyCode,
                         CGEventFlags& outFlags);

#endif // KEEPASSXC_AUTOTYPEMAC_P_H
