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

#ifndef KEEPASSX_KEEPASS2XMLREADER_H
#define KEEPASSX_KEEPASS2XMLREADER_H

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QXmlStreamReader>
#include <QtGui/QColor>

#include "core/TimeInfo.h"
#include "core/Uuid.h"

class Database;
class Entry;
class Group;
class Metadata;

class KeePass2XmlReader
{
    Q_DECLARE_TR_FUNCTIONS(KeePass2XmlReader);

public:
    KeePass2XmlReader();
    Database* readDatabase(QIODevice* device);
    void readDatabase(QIODevice* device, Database* db);
    Database* readDatabase(const QString& filename);
    bool error();
    QString errorString();

private:
    void parseKeePassFile();
    void parseMeta();
    void parseMemoryProtection();
    void parseCustomIcons();
    void parseIcon();
    void parseCustomData();
    void parseCustomDataItem();
    void parseRoot();
    Group* parseGroup();
    void parseDeletedObjects();
    void parseDeletedObject();
    Entry* parseEntry(bool history);
    void parseEntryString(Entry* entry);
    void parseEntryBinary(Entry* entry);
    void parseAutoType(Entry* entry);
    void parseAutoTypeAssoc(Entry* entry);
    void parseEntryHistory(Entry* entry);
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

#endif // KEEPASSX_KEEPASS2XMLREADER_H
