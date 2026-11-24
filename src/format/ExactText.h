/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_EXACTTEXT_H
#define KEEPASSX_EXACTTEXT_H

#include <QByteArray>
#include <QChar>
#include <QString>

// turning text into bytes and back so that exactly the same text comes out.
//
// QString::fromUtf8() takes a leading U+FEFF for a byte order mark and silently drops it, so a password that really
// starts with that (invisible) character came back from a saved database without it. toUtf8() turns a lone surrogate
// (one half of an emoji) into a replacement character. These helpers keep the first and make the second detectable.
namespace ExactText
{
    inline QString fromUtf8(const QByteArray& bytes)
    {
        qsizetype start = 0;
        while (bytes.size() - start >= 3 && bytes.at(start) == '\xEF' && bytes.at(start + 1) == '\xBB'
               && bytes.at(start + 2) == '\xBF') {
            start += 3;
        }
        // What is left does not start with a byte order mark, so fromUtf8 keeps all of it
        return QString(start / 3, QChar(0xFEFF)) + QString::fromUtf8(bytes.constData() + start, bytes.size() - start);
    }

    // True when the text comes back unchanged after toUtf8() and fromUtf8() (false for a lone surrogate)
    inline bool utf8RoundTrips(const QString& text)
    {
        return fromUtf8(text.toUtf8()) == text;
    }

    // UTF-16, little endian: every possible text, lone surrogates included
    inline QByteArray toUtf16Le(const QString& text)
    {
        QByteArray bytes;
        bytes.reserve(text.size() * 2);
        for (const QChar ch : text) {
            bytes.append(static_cast<char>(ch.unicode() & 0xFF));
            bytes.append(static_cast<char>(ch.unicode() >> 8));
        }
        return bytes;
    }

    // False (and text empty) when the number of bytes is odd
    inline bool fromUtf16Le(const QByteArray& bytes, QString& text)
    {
        text.clear();
        if (bytes.size() % 2 != 0) {
            return false;
        }
        text.reserve(bytes.size() / 2);
        for (qsizetype i = 0; i < bytes.size(); i += 2) {
            text.append(QChar(
                static_cast<ushort>(static_cast<uchar>(bytes.at(i)) | (static_cast<uchar>(bytes.at(i + 1)) << 8))));
        }
        return true;
    }
} // namespace ExactText

#endif // KEEPASSX_EXACTTEXT_H
