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
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QXmlStreamReader>
#include <QtGui/QColor>

#include "core/Global.h"
#include "core/TimeInfo.h"
#include "core/Uuid.h"

class Database;
class Entry;
class Group;
class KeePass2RandomStream;
class Metadata;

class KeePass2XmlReader
{
    Q_DECLARE_TR_FUNCTIONS(KeePass2XmlReader)

public:
    KeePass2XmlReader();
    Database* readDatabase(QIODevice* device);
    void readDatabase(QIODevice* device, Database* db, KeePass2RandomStream* randomStream = Q_NULLPTR);
    Database* readDatabase(const QString& filename);
    bool hasError();
    QString errorString();
    QByteArray headerHash();

private:
    void parseKeePassFile();
    void parseMeta();
    void parseMemoryProtection();
    void parseCustomIcons();
    void parseIcon();
    void parseBinaries();
    void parseCustomData();
    void parseCustomDataItem();
    void parseRoot();
    Group* parseGroup();
    void parseDeletedObjects();
    void parseDeletedObject();
    Entry* parseEntry(bool history);
    void parseEntryString(Entry* entry);
    QPair<QString, QString> parseEntryBinary(Entry* entry);
    void parseAutoType(Entry* entry);
    void parseAutoTypeAssoc(Entry* entry);
    QList<Entry*> parseEntryHistory();
    TimeInfo parseTimes();

    QString readString();
    bool readBool();
    QDateTime readDateTime();
    QColor readColor();
    int readNumber();
    Uuid readUuid();
    QByteArray readBinary();
    QByteArray readCompressedBinary();

    Group* getGroup(const Uuid& uuid);
    Entry* getEntry(const Uuid& uuid);
    void raiseError(int internalNumber);
    void skipCurrentElement();

    QXmlStreamReader m_xml;
    KeePass2RandomStream* m_randomStream;
    Database* m_db;
    Metadata* m_meta;
    Group* m_tmpParent;
    QHash<Uuid, Group*> m_groups;
    QHash<Uuid, Entry*> m_entries;
    QHash<QString, QByteArray> m_binaryPool;
    QHash<QString, QPair<Entry*, QString> > m_binaryMap;
    QByteArray m_headerHash;
};

#endif // KEEPASSX_KEEPASS2XMLREADER_H
