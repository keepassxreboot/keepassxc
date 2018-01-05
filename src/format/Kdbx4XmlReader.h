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

#ifndef KEEPASSX_KDBX4XMLREADER_H
#define KEEPASSX_KDBX4XMLREADER_H

#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QHash>
#include <QPair>
#include <QXmlStreamReader>

#include "core/TimeInfo.h"
#include "core/Uuid.h"

class Database;
class Entry;
class Group;
class KeePass2RandomStream;
class Metadata;

class Kdbx4XmlReader
{
    Q_DECLARE_TR_FUNCTIONS(Kdbx4XmlReader)

public:
    Kdbx4XmlReader();
    Kdbx4XmlReader(QHash<QString, QByteArray>& binaryPool);
    Database* readDatabase(QIODevice* device);
    void readDatabase(QIODevice* device, Database* db, KeePass2RandomStream* randomStream = nullptr);
    Database* readDatabase(const QString& filename);
    bool hasError();
    QString errorString();
    QByteArray headerHash();
    void setStrictMode(bool strictMode);

private:
    bool parseKeePassFile();
    void parseMeta();
    void parseMemoryProtection();
    void parseCustomIcons();
    void parseIcon();
    void parseBinaries();
    void parseCustomData();
    void parseCustomDataItem();
    bool parseRoot();
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
    void raiseError(const QString& errorMessage);
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
    bool m_error;
    QString m_errorStr;
    bool m_strictMode;
};

#endif // KEEPASSX_KDBX4XMLREADER_H
