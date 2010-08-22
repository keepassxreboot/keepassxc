/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_WRITER_H
#define KEEPASSX_WRITER_H

#include <QtCore/QDateTime>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QColor>
#include <QtGui/QImage>

#include "core/Entry.h"
#include "core/TimeInfo.h"
#include "core/Uuid.h"

class Database;
class Group;
class Metadata;

class Writer : public QObject
{
    Q_OBJECT

public:
    Writer(Database* db);
    void write(QIODevice* device);
    void write(const QString& filename);

private:
    void writeMetadata();
    void writeMemoryProtection();
    void writeCustomIcons();
    void writeIcon(const Uuid& uuid, const QImage& image);
    void writeCustomData();
    void writeRoot();
    void writeGroup(const Group* group);
    void writeTimes(const TimeInfo& ti);
    void writeEntry(const Entry* entry);
    void writeAutoType(const Entry* entry);
    void writeAutoTypeAssoc(const AutoTypeAssociation& assoc);
    void writeEntryHistory(const Entry* entry);

    void writeString(const QString& qualifiedName, const QString& string);
    void writeNumber(const QString& qualifiedName, int number);
    void writeBool(const QString& qualifiedName, bool b);
    void writeDateTime(const QString& qualifiedName, const QDateTime& dateTime);
    void writeUuid(const QString& qualifiedName, const Uuid& uuid);
    void writeUuid(const QString& qualifiedName, const Group* group);
    void writeUuid(const QString& qualifiedName, const Entry* entry);
    void writeBinary(const QString& qualifiedName, const QByteArray& ba);
    void writeColor(const QString& qualifiedName, const QColor& color);
    QString colorPartToString(int value);

    QXmlStreamWriter m_xml;
    Database* m_db;
    Metadata* m_meta;
};

#endif // KEEPASSX_WRITER_H
