/*
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

#include "Tools.h"

#include <QtCore/QIODevice>
#include <QtCore/QLocale>
#include <QtCore/QStringList>

namespace Tools {

QString humanReadableFileSize(qint64 bytes)
{
    double size = bytes;

    QStringList units = QStringList() << "B" << "KiB" << "MiB" << "GiB";
    int i = 0;
    int maxI = units.size() - 1;

    while ((size >= 1024) && (i < maxI)) {
        size /= 1024;
        i++;
    }

    return QString("%1 %2").arg(QLocale().toString(size, 'f', 2), units.at(i));
}

bool hasChild(const QObject* parent, const QObject* child)
{
    if (!parent || !child) {
        return false;
    }
    Q_FOREACH (QObject* c, parent->children()) {
        if (child == c || hasChild(c, child)) {
            return true;
        }
    }
    return false;
}

bool readAllFromDevice(QIODevice* device, QByteArray& data)
{
    QByteArray result;
    qint64 readBytes = 0;
    qint64 readResult;
    do {
        result.resize(result.size() + 16384);
        readResult = device->read(result.data() + readBytes, result.size() - readBytes);
        if (readResult > 0) {
            readBytes += readResult;
        }
    } while (readResult > 0);

    if (readResult == -1) {
        return false;
    }
    else {
        result.resize(static_cast<int>(readBytes));
        data = result;
        return true;
    }
}

QDateTime currentDateTimeUtc ()
{
#if QT_VERSION >= 0x040700
     return QDateTime::currentDateTimeUtc();
#else
     return QDateTime::currentDateTime().toUTC();
#endif
}

} // namespace Tools
