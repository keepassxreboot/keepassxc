/*
 *  Copyright (C) 2015 Florian Geyer <blueice@fobos.de>
 *  Copyright (C) 2015 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2026 Efraim Flashner <efraim@flashner.co.il>
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

#include "NetrcExporter.h"

#include <QFile>

#include "core/Group.h"

bool NetrcExporter::exportDatabase(const QString& filename, const QSharedPointer<const Database>& db)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_error = file.errorString();
        return false;
    }
    return exportDatabase(&file, db);
}

bool NetrcExporter::exportDatabase(QIODevice* device, const QSharedPointer<const Database>& db)
{
    if (device->write(exportGroup(db->rootGroup()).toUtf8()) == -1) {
        m_error = device->errorString();
        return false;
    }

    return true;
}

QString NetrcExporter::exportDatabase(const QSharedPointer<const Database>& db)
{
    return exportGroup(db->rootGroup());
}

QString NetrcExporter::errorString() const
{
    return m_error;
}

QString NetrcExporter::exportGroup(const Group* group, QString groupPath)
{
    QString response;

    const QList<Entry*>& entryList = group->entries();
    for (const Entry* entry : entryList) {
        QString line;

        if (!(entry->title().isEmpty())) {
            addColumn(line, "machine");
            addColumn(line, entry->title());
            addColumn(line, "login");
            addColumn(line, entry->username());
            addColumn(line, "password");
            addColumn(line, entry->password());
            line.append("\n");
        }

        // Add it a second time using the URL
        if (!(entry->url().isEmpty())) {
            addColumn(line, "machine");
            addColumn(line, entry->url());
            addColumn(line, "login");
            addColumn(line, entry->username());
            addColumn(line, "password");
            addColumn(line, entry->password());
            line.append("\n");
        }

        response.append(line);
    }

    const QList<Group*>& children = group->children();
    for (const Group* child : children) {
        response.append(exportGroup(child, groupPath));
    }

    return response;
}

void NetrcExporter::addColumn(QString& str, const QString& column)
{
    str.append(QString(column).replace("\"", "\"\""));
    str.append("\t");
}
