/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_KDBXXMLWRITER_H
#define KEEPASSX_KDBXXMLWRITER_H

#include <QDateTime>
#include <QSet>
#include <QStringList>
#include <QXmlStreamWriter>

#include "core/CustomData.h"
#include "core/Group.h"
#include "core/Metadata.h"

class KeePass2RandomStream;

class KdbxXmlWriter
{
public:
    /**
     * Map of entry + attachment key to KDBX 4 inner header binary index.
     */
    typedef QHash<QPair<const Entry*, QString>, qint64> BinaryIdxMap;

    explicit KdbxXmlWriter(quint32 version);
    explicit KdbxXmlWriter(quint32 version, KdbxXmlWriter::BinaryIdxMap binaryIdxMap);

    void writeDatabase(QIODevice* device,
                       const Database* db,
                       KeePass2RandomStream* randomStream = nullptr,
                       const QByteArray& headerHash = QByteArray());
    void writeDatabase(const QString& filename, Database* db);
    void disableInnerStreamProtection(bool disable);
    bool innerStreamProtectionDisabled() const;
    // also write the attachment pool (Meta/Binaries) for KDBX 4+ so a standalone XML export contains the
    // attachment content. KDBX 4 files keep attachments in the inner header, which a bare XML export does not have.
    void setWriteBinaryPool(bool enable);
    // write carriage returns as character references. A raw CR is normalized to LF by every XML parser, so
    // Windows line breaks in notes would silently change in an XML export.
    void setPreserveCarriageReturns(bool enable);
    // text XML 1.0 cannot hold (control characters, NUL, lone surrogates) is stored as base64 and the element
    // gets a Base64 attribute naming how the text was encoded, instead of the characters being deleted. Only meant for
    // the standalone XML export: a database file is still written the old way, which other KeePass programs read.
    void setStoreHiddenCharactersAsBase64(bool enable);
    // after writing, one line per place whose text was stored as base64, e.g.
    // Entry "Bank" in folder "Personal": Password. Names the field, never shows the value.
    QStringList hiddenCharacterPlaces() const;
    bool hasError();
    QString errorString();

private:
    void fillBinaryIdxMap();

    void writeMetadata();
    void writeMemoryProtection();
    void writeCustomIcons();
    void writeIcon(const QUuid& uuid, const Metadata::CustomIconData& iconData);
    void writeBinaries();
    void writeCustomData(const CustomData* customData, bool writeItemLastModified = false);
    void
    writeCustomDataItem(const QString& key, const CustomData::CustomDataItem& item, bool writeLastModified = false);
    void writeRoot();
    void writeGroup(const Group* group);
    void writeTimes(const TimeInfo& ti);
    void writeDeletedObjects();
    void writeDeletedObject(const DeletedObject& delObj);
    void writeEntry(const Entry* entry);
    void writeAutoType(const Entry* entry);
    void writeAutoTypeAssoc(const AutoTypeAssociations::Association& assoc);
    void writeEntryHistory(const Entry* entry);

    void writeText(const QString& text);
    void writeString(const QString& qualifiedName, const QString& string);
    void writeNumber(const QString& qualifiedName, int number);
    void writeBool(const QString& qualifiedName, bool b);
    void writeDateTime(const QString& qualifiedName, const QDateTime& dateTime);
    void writeUuid(const QString& qualifiedName, const QUuid& uuid);
    void writeUuid(const QString& qualifiedName, const Group* group);
    void writeUuid(const QString& qualifiedName, const Entry* entry);
    void writeBinary(const QString& qualifiedName, const QByteArray& ba);
    void writeBase64Characters(const QByteArray& data);
    void writeTriState(const QString& qualifiedName, Group::TriState triState);
    QString colorPartToString(int value);
    QString stripInvalidXml10Chars(QString str);
    static bool isInvalidXml10Unit(QChar ch);
    static bool hasInvalidXml10Chars(const QString& str);
    void writeBase64Text(const QString& text);
    static QString printableText(const QString& text);

    void raiseError(const QString& errorMessage);

    const quint32 m_kdbxVersion;

    bool m_innerStreamProtectionDisabled = false;
    bool m_writeBinaryPool = false;
    bool m_preserveCarriageReturns = false;
    bool m_hiddenCharactersAsBase64 = false;
    // What is being written right now, for the list of places stored as base64
    QString m_place;
    QString m_field;
    QStringList m_hiddenCharacterPlaces;
    QSet<QString> m_hiddenCharacterPlaceSet;

    QXmlStreamWriter m_xml;
    QPointer<const Database> m_db;
    QPointer<const Metadata> m_meta;
    KeePass2RandomStream* m_randomStream = nullptr;
    BinaryIdxMap m_binaryIdxMap;
    QByteArray m_headerHash;

    bool m_error = false;

    QString m_errorStr = "";
};

#endif // KEEPASSX_KDBXXMLWRITER_H
