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

#ifndef KEEPASSX_PARSER_H
#define KEEPASSX_PARSER_H

#include <QtCore/QDateTime>
#include <QtCore/QXmlStreamReader>
#include <QtGui/QColor>

#include "TimeInfo.h"
#include "Uuid.h"

class Database;
class Entry;
class Group;
class Metadata;

class Parser : public QObject
{
    Q_OBJECT

public:
    Parser(Database* db);
    bool parse(const QString& filename);
    QString errorMsg();

private:
    void parseKeePassFile();
    void parseMeta();
    void parseMemoryProtection();
    void parseCustomIcons();
    void parseIcon();
    void parseCustomData();
    void parseRoot();
    Group* parseGroup();
    Entry* parseEntry();
    void parseEntryString(Entry* entry);
    void parseEntryBinary(Entry* entry);
    void parseAutoType(Entry* entry);
    void parseAutoTypeAssoc(Entry* entry);
    void parseEntryHistory();
    TimeInfo parseTimes();

    QString readString();
    bool readBool();
    QDateTime readDateTime();
    QColor readColor();
    int readNumber();
    Uuid readUuid();
    QByteArray readBinary();

    Group* getGroup(const Uuid& uuid);
    Entry* getEntry(const Uuid& uuid);
    void raiseError();
    void skipCurrentElement();

    QXmlStreamReader m_xml;
    Database* m_db;
    Metadata* m_meta;
    Group* m_tmpParent;
    QList<Group*> m_groups;
    QList<Entry*> m_entries;
};

#endif // KEEPASSX_PARSER_H
