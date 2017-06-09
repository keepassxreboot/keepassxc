/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_TOOLS_H
#define KEEPASSX_TOOLS_H

#include "core/Global.h"

#include <QDateTime>
#include <QObject>
#include <QString>

#include <algorithm>

class QIODevice;

namespace Tools {

QString humanReadableFileSize(qint64 bytes);
bool hasChild(const QObject* parent, const QObject* child);
bool readFromDevice(QIODevice* device, QByteArray& data, int size = 16384);
bool readAllFromDevice(QIODevice* device, QByteArray& data);
QString imageReaderFilter();
bool isHex(const QByteArray& ba);
bool isBase64(const QByteArray& ba);
void sleep(int ms);
void wait(int ms);
void disableCoreDumps();
void setupSearchPaths();
bool createWindowsDACL();

template <typename RandomAccessIterator, typename T>
RandomAccessIterator binaryFind(RandomAccessIterator begin, RandomAccessIterator end, const T& value)
{
    RandomAccessIterator it = std::lower_bound(begin, end, value);

    if ((it == end) || (value < *it)) {
        return end;
    }
    else {
        return it;
    }
}

} // namespace Tools

#endif // KEEPASSX_TOOLS_H
