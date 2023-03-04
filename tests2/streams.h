/*
*  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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
#ifndef KEEPASSXC_STREAMS_H
#define KEEPASSXC_STREAMS_H

#include <iostream>
#include <QByteArray>
#include <QLatin1String>
#include <QString>

// To fix the ?????? output on QT string assertions.
// Origin: https://dzone.com/articles/unit-tests-for-qt-based-applications-with-catch
inline std::ostream &operator<<(std::ostream &os, const QByteArray &value)
{
    return os << '"' << (value.isEmpty() ? "" : value.constData()) << '"';
}

inline std::ostream &operator<<(std::ostream &os, const QLatin1String &value)
{
    return os << '"' << value.latin1() << '"';
}

inline std::ostream &operator<<(std::ostream &os, const QString &value)
{
    return os << value.toLocal8Bit();
}

#endif // KEEPASSXC_STREAMS_H