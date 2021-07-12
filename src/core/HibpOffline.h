/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_HIBPOFFLINE_H
#define KEEPASSXC_HIBPOFFLINE_H

#include <QSharedPointer>

class QIODevice;

class Database;
class Entry;

namespace HibpOffline
{
    bool report(QSharedPointer<Database> db,
                QIODevice& hibpInput,
                QList<QPair<const Entry*, int>>& findings,
                QString* error);

    bool okonReport(QSharedPointer<Database> db,
                    const QString& okon,
                    const QString& okonDatabase,
                    QList<QPair<const Entry*, int>>& findings,
                    QString* error);
} // namespace HibpOffline

#endif // KEEPASSXC_HIBPOFFLINE_H
