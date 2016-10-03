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

#include "CsvExporter.h"

#include <QFile>

#include "core/Database.h"
#include "core/Group.h"

bool CsvExporter::exportDatabase(const QString& filename, const Database* db)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_error = file.errorString();
        return false;
    }
    return exportDatabase(&file, db);
}

bool CsvExporter::exportDatabase(QIODevice* device, const Database* db)
{
    QString header;
    addColumn(header, "Group");
    addColumn(header, "Title");
    addColumn(header, "Username");
    addColumn(header, "Password");
    addColumn(header, "URL");
    addColumn(header, "Notes");
    header.append("\n");

    if (device->write(header.toUtf8()) == -1) {
        m_error = device->errorString();
        return false;
    }

    return writeGroup(device, db->rootGroup());
}

QString CsvExporter::errorString() const
{
    return m_error;
}

bool CsvExporter::writeGroup(QIODevice* device, const Group* group, QString groupPath)
{
    if (!groupPath.isEmpty()) {
        groupPath.append("/");
    }
    groupPath.append(group->name());

    const QList<Entry*> entryList = group->entries();
    for (const Entry* entry : entryList) {
        QString line;

        addColumn(line, groupPath);
        addColumn(line, entry->title());
        addColumn(line, entry->username());
        addColumn(line, entry->password());
        addColumn(line, entry->url());
        addColumn(line, entry->notes());

        line.append("\n");

        if (device->write(line.toUtf8()) == -1) {
            m_error = device->errorString();
            return false;
        }
    }

    const QList<Group*> children = group->children();
    for (const Group* child : children) {
        if (!writeGroup(device, child, groupPath)) {
            return false;
        }
    }

    return true;
}

void CsvExporter::addColumn(QString& str, const QString& column)
{
    if (!str.isEmpty()) {
        str.append(",");
    }

    str.append("\"");
    str.append(QString(column).replace("\"", "\"\""));
    str.append("\"");
}
