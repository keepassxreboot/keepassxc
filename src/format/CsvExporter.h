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

#include <QSharedPointer>
#include <QString>
#include <QStringList>

class Database;
class Entry;
class Group;
class QIODevice;

class CsvExporter
{
public:
    bool exportDatabase(const QString& filename, const QSharedPointer<const Database>& db);
    bool exportDatabase(QIODevice* device, const QSharedPointer<const Database>& db);
    QString exportDatabase(const QSharedPointer<const Database>& db);
    QString errorString() const;

    // what a CSV file cannot hold of this database, one sentence each, naming the folders and entries
    // affected. To be read by the person before the file is saved. The first sentence (the settings of the database
    // itself) is always there. Nothing is changed.
    static QStringList lossReport(const Database& db);

private:
    QString exportGroup(const Group* group, QString groupPath = QString());
    bool writeGroup(QIODevice* device, const Group* group, QString groupPath);
    bool writeText(QIODevice* device, const QString& text);
    QString exportEntry(const Entry* entry, const QString& groupPath);
    QString exportHeader();
    void addColumn(QString& str, const QString& column);

    QString m_error;
};

#endif // KEEPASSX_CSVEXPORTER_H
