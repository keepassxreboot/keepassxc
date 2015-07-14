/*
 *  Copyright (C) 2015 Florian Geyer <blueice@fobos.de>
 *  Copyright (C) 2015 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_CSVEXPORTER_H
#define KEEPASSX_CSVEXPORTER_H

#include <QString>

class Database;
class Group;
class QIODevice;

class CsvExporter
{
public:
    bool exportDatabase(const QString& filename, const Database* db);
    bool exportDatabase(QIODevice* device, const Database* db);
    QString errorString() const;

private:
    bool writeGroup(QIODevice* device, const Group* group, QString groupPath = QString());
    void addColumn(QString& str, const QString& column);

    QString m_error;
};

#endif // KEEPASSX_CSVEXPORTER_H
