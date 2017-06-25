// Base32 implementation
// Source: https://github.com/google/google-authenticator-libpam/blob/master/src/base32.c
//
// Copyright 2010 Google Inc.
// Author: Markus Gutschke
// Modifications Copyright 2017 KeePassXC team <team@keepassxc.org>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "base32.h"

Base32::Base32()
{
}

QByteArray Base32::base32_decode(const QByteArray encoded)
{
    QByteArray result;

    int buffer = 0;
    int bitsLeft = 0;

    for (char ch : encoded) {
        if (ch == 0 || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-' || ch == '=') {
            continue;
        }

        buffer <<= 5;

        // Deal with commonly mistyped characters
        if (ch == '0') {
            ch = 'O';
        } else if (ch == '1') {
            ch = 'L';
        } else if (ch == '8') {
            ch = 'B';
        }

        // Look up one base32 digit
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
            ch = (ch & 0x1F) - 1;
        } else if (ch >= '2' && ch <= '7') {
            ch -= '2' - 26;
        } else {
            return QByteArray();
        }

        buffer |= ch;
        bitsLeft += 5;

        if (bitsLeft >= 8) {
            result.append(static_cast<char> (buffer >> (bitsLeft - 8)));
            bitsLeft -= 8;
        }
    }

    return result;
}