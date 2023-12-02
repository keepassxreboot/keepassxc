/*
 * Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "KdbxXmlReader.h"
#include "KeePass2RandomStream.h"
#include "core/Clock.h"
#include "core/Endian.h"
#include "core/Global.h"
#include "core/Group.h"
#include "core/Tools.h"
#include "streams/qtiocompressor.h"

#include <QBuffer>
#include <QFile>

#define UUID_LENGTH 16

/**
 * @param version KDBX version
 */
KdbxXmlReader::KdbxXmlReader(quint32 version)
    : m_kdbxVersion(version)
{
}

/**
 * @param version KDBX version
 * @param binaryPool binary pool
 */
KdbxXmlReader::KdbxXmlReader(quint32 version, QHash<QString, QByteArray> binaryPool)
    : m_kdbxVersion(version)
    , m_binaryPool(std::move(binaryPool))
{
}

/**
 * Read XML contents from a file into a new database.
 *
 * @param device input file
 * @return pointer to the new database
 */
QSharedPointer<Database> KdbxXmlReader::readDatabase(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    return readDatabase(&file);
}

/**
 * Read XML stream from a device into a new database.
 *
 * @param device input device
 * @return pointer to the new database
 */
QSharedPointer<Database> KdbxXmlReader::readDatabase(QIODevice* device)
{
    auto db = QSharedPointer<Database>::create();
    readDatabase(device, db.data());
    return db;
}

/**
 * Read XML contents from a device into a given database using a \link KeePass2RandomStream.
 *
 * @param device input device
 * @param db database to read into
 * @param randomStream random stream to use for decryption
 */
void KdbxXmlReader::readDatabase(QIODevice* device, Database* db, KeePass2RandomStream* randomStream)
{
    m_error = false;
    m_errorStr.clear();

    m_xml.clear();
    m_xml.setDevice(device);

    m_db = db;
    m_meta = m_db->metadata();
    m_meta->setUpdateDatetime(false);

    m_randomStream = randomStream;
    m_headerHash.clear();

    m_tmpParent.reset(new Group());

    bool rootGroupParsed = false;

    if (m_xml.hasError()) {
        raiseError(tr("XML parsing failure: %1").arg(m_xml.error()));
        return;
    }

    if (m_xml.readNextStartElement() && m_xml.name().compare("KeePassFile") == 0) {
        rootGroupParsed = parseKeePassFile();
    }

    if (!rootGroupParsed) {
        raiseError(tr("No root group"));
        return;
    }

    if (!m_tmpParent->children().isEmpty()) {
        qWarning("KdbxXmlReader::readDatabase: found %" PRIdQSIZETYPE " invalid group reference(s)",
                 m_tmpParent->children().size());
    }

    if (!m_tmpParent->entries().isEmpty()) {
        qWarning("KdbxXmlReader::readDatabase: found %" PRIdQSIZETYPE " invalid entry reference(s)",
                 m_tmpParent->children().size());
    }

    const QSet<QString> poolKeys = Tools::asSet(m_binaryPool.keys());
    const QSet<QString> entryKeys = Tools::asSet(m_binaryMap.keys());
    const QSet<QString> unmappedKeys = entryKeys - poolKeys;
    const QSet<QString> unusedKeys = poolKeys - entryKeys;

    if (!unmappedKeys.isEmpty()) {
        qWarning("Unmapped keys left.");
    }

    for (const QString& key : unusedKeys) {
        qWarning("KdbxXmlReader::readDatabase: found unused key \"%s\"", qPrintable(key));
    }

    QMultiHash<QString, QPair<Entry*, QString>>::const_iterator i;
    for (i = m_binaryMap.constBegin(); i != m_binaryMap.constEnd(); ++i) {
        const QPair<Entry*, QString>& target = i.value();
        target.first->attachments()->set(target.second, m_binaryPool[i.key()]);
    }

    m_meta->setUpdateDatetime(true);

    QHash<QUuid, Group*>::const_iterator iGroup;
    for (iGroup = m_groups.constBegin(); iGroup != m_groups.constEnd(); ++iGroup) {
        iGroup.value()->setUpdateTimeinfo(true);
    }

    QHash<QUuid, Entry*>::const_iterator iEntry;
    for (iEntry = m_entries.constBegin(); iEntry != m_entries.constEnd(); ++iEntry) {
        iEntry.value()->setUpdateTimeinfo(true);

        const QList<Entry*> historyItems = iEntry.value()->historyItems();
        for (Entry* histEntry : historyItems) {
            histEntry->setUpdateTimeinfo(true);
        }
    }
}

bool KdbxXmlReader::strictMode() const
{
    return m_strictMode;
}

void KdbxXmlReader::setStrictMode(bool strictMode)
{
    m_strictMode = strictMode;
}

bool KdbxXmlReader::hasError() const
{
    return m_error || m_xml.hasError();
}

QString KdbxXmlReader::errorString() const
{
    if (m_error) {
        return m_errorStr;
    }
    if (m_xml.hasError()) {
        return tr("XML error:\n%1\nLine %2, column %3")
            .arg(m_xml.errorString())
            .arg(m_xml.lineNumber())
            .arg(m_xml.columnNumber());
    }
    return {};
}

bool KdbxXmlReader::isTrueValue(const QStringView& value)
{
    return value.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0 || value.compare("1") == 0;
}

void KdbxXmlReader::raiseError(const QString& errorMessage)
{
    m_error = true;
    m_errorStr = errorMessage;
}

QByteArray KdbxXmlReader::headerHash() const
{
    return m_headerHash;
}

bool KdbxXmlReader::parseKeePassFile()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("KeePassFile") == 0);

    bool rootElementFound = false;
    bool rootParsedSuccessfully = false;

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Meta") == 0) {
            parseMeta();
            continue;
        }

        if (m_xml.name().compare("Root") == 0) {
            if (rootElementFound) {
                rootParsedSuccessfully = false;
                qWarning("Multiple root elements");
            } else {
                rootParsedSuccessfully = parseRoot();
                rootElementFound = true;
            }
            continue;
        }

        skipCurrentElement();
    }

    return rootParsedSuccessfully;
}

void KdbxXmlReader::parseMeta()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("Meta") == 0);

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Generator") == 0) {
            m_meta->setGenerator(readString());
        } else if (m_xml.name().compare("HeaderHash") == 0) {
            m_headerHash = readBinary();
        } else if (m_xml.name().compare("DatabaseName") == 0) {
            m_meta->setName(readString());
        } else if (m_xml.name().compare("DatabaseNameChanged") == 0) {
            m_meta->setNameChanged(readDateTime());
        } else if (m_xml.name().compare("DatabaseDescription") == 0) {
            m_meta->setDescription(readString());
        } else if (m_xml.name().compare("DatabaseDescriptionChanged") == 0) {
            m_meta->setDescriptionChanged(readDateTime());
        } else if (m_xml.name().compare("DefaultUserName") == 0) {
            m_meta->setDefaultUserName(readString());
        } else if (m_xml.name().compare("DefaultUserNameChanged") == 0) {
            m_meta->setDefaultUserNameChanged(readDateTime());
        } else if (m_xml.name().compare("MaintenanceHistoryDays") == 0) {
            m_meta->setMaintenanceHistoryDays(readNumber());
        } else if (m_xml.name().compare("Color") == 0) {
            m_meta->setColor(readColor());
        } else if (m_xml.name().compare("MasterKeyChanged") == 0) {
            m_meta->setDatabaseKeyChanged(readDateTime());
        } else if (m_xml.name().compare("MasterKeyChangeRec") == 0) {
            m_meta->setMasterKeyChangeRec(readNumber());
        } else if (m_xml.name().compare("MasterKeyChangeForce") == 0) {
            m_meta->setMasterKeyChangeForce(readNumber());
        } else if (m_xml.name().compare("MemoryProtection") == 0) {
            parseMemoryProtection();
        } else if (m_xml.name().compare("CustomIcons") == 0) {
            parseCustomIcons();
        } else if (m_xml.name().compare("RecycleBinEnabled") == 0) {
            m_meta->setRecycleBinEnabled(readBool());
        } else if (m_xml.name().compare("RecycleBinUUID") == 0) {
            m_meta->setRecycleBin(getGroup(readUuid()));
        } else if (m_xml.name().compare("RecycleBinChanged") == 0) {
            m_meta->setRecycleBinChanged(readDateTime());
        } else if (m_xml.name().compare("EntryTemplatesGroup") == 0) {
            m_meta->setEntryTemplatesGroup(getGroup(readUuid()));
        } else if (m_xml.name().compare("EntryTemplatesGroupChanged") == 0) {
            m_meta->setEntryTemplatesGroupChanged(readDateTime());
        } else if (m_xml.name().compare("LastSelectedGroup") == 0) {
            m_meta->setLastSelectedGroup(getGroup(readUuid()));
        } else if (m_xml.name().compare("LastTopVisibleGroup") == 0) {
            m_meta->setLastTopVisibleGroup(getGroup(readUuid()));
        } else if (m_xml.name().compare("HistoryMaxItems") == 0) {
            int value = readNumber();
            if (value >= -1) {
                m_meta->setHistoryMaxItems(value);
            } else {
                qWarning("HistoryMaxItems invalid number");
            }
        } else if (m_xml.name().compare("HistoryMaxSize") == 0) {
            int value = readNumber();
            if (value >= -1) {
                m_meta->setHistoryMaxSize(value);
            } else {
                qWarning("HistoryMaxSize invalid number");
            }
        } else if (m_xml.name().compare("Binaries") == 0) {
            parseBinaries();
        } else if (m_xml.name().compare("CustomData") == 0) {
            parseCustomData(m_meta->customData());
        } else if (m_xml.name().compare("SettingsChanged") == 0) {
            m_meta->setSettingsChanged(readDateTime());
        } else {
            skipCurrentElement();
        }
    }
}

void KdbxXmlReader::parseMemoryProtection()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("MemoryProtection") == 0);

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("ProtectTitle") == 0) {
            m_meta->setProtectTitle(readBool());
        } else if (m_xml.name().compare("ProtectUserName") == 0) {
            m_meta->setProtectUsername(readBool());
        } else if (m_xml.name().compare("ProtectPassword") == 0) {
            m_meta->setProtectPassword(readBool());
        } else if (m_xml.name().compare("ProtectURL") == 0) {
            m_meta->setProtectUrl(readBool());
        } else if (m_xml.name().compare("ProtectNotes") == 0) {
            m_meta->setProtectNotes(readBool());
        } else {
            skipCurrentElement();
        }
    }
}

void KdbxXmlReader::parseCustomIcons()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("CustomIcons") == 0);

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Icon") == 0) {
            parseIcon();
        } else {
            skipCurrentElement();
        }
    }
}

void KdbxXmlReader::parseIcon()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("Icon") == 0);

    QUuid uuid;
    QByteArray iconData;
    QString name;
    QDateTime lastModified;
    bool uuidSet = false;
    bool iconSet = false;

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("UUID") == 0) {
            uuid = readUuid();
            uuidSet = !uuid.isNull();
        } else if (m_xml.name().compare("Data") == 0) {
            iconData = readBinary();
            iconSet = true;
        } else if (m_xml.name().compare("Name") == 0) {
            name = readString();
        } else if (m_xml.name().compare("LastModificationTime") == 0) {
            lastModified = readDateTime();
        } else {
            skipCurrentElement();
        }
    }

    if (uuidSet && iconSet) {
        // Check for duplicate UUID (corruption)
        if (m_meta->hasCustomIcon(uuid)) {
            uuid = QUuid::createUuid();
        }
        m_meta->addCustomIcon(uuid, iconData, name, lastModified);
        return;
    }

    raiseError(tr("Missing icon uuid or data"));
}

void KdbxXmlReader::parseBinaries()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("Binaries") == 0);

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Binary") != 0) {
            skipCurrentElement();
            continue;
        }

        QXmlStreamAttributes attr = m_xml.attributes();
        QString id = attr.value("ID").toString();
        QByteArray data = isTrueValue(attr.value("Compressed")) ? readCompressedBinary() : readBinary();

        if (m_binaryPool.contains(id)) {
            qWarning("KdbxXmlReader::parseBinaries: overwriting binary item \"%s\"", qPrintable(id));
        }

        m_binaryPool.insert(id, data);
    }
}

void KdbxXmlReader::parseCustomData(CustomData* customData)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("CustomData") == 0);

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Item") == 0) {
            parseCustomDataItem(customData);
            continue;
        }
        skipCurrentElement();
    }
}

void KdbxXmlReader::parseCustomDataItem(CustomData* customData)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("Item") == 0);

    QString key;
    CustomData::CustomDataItem item;
    bool keySet = false;
    bool valueSet = false;

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Key") == 0) {
            key = readString();
            keySet = true;
        } else if (m_xml.name().compare("Value") == 0) {
            item.value = readString();
            valueSet = true;
        } else if (m_xml.name().compare("LastModificationTime") == 0) {
            item.lastModified = readDateTime();
        } else {
            skipCurrentElement();
        }
    }

    if (keySet && valueSet) {
        customData->set(key, item);
        return;
    }

    raiseError(tr("Missing custom data key or value"));
}

bool KdbxXmlReader::parseRoot()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("Root") == 0);

    bool groupElementFound = false;
    bool groupParsedSuccessfully = false;

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Group") == 0) {
            if (groupElementFound) {
                groupParsedSuccessfully = false;
                raiseError(tr("Multiple group elements"));
                continue;
            }

            Group* rootGroup = parseGroup();
            if (rootGroup) {
                Group* oldRoot = m_db->rootGroup();
                m_db->setRootGroup(rootGroup);
                delete oldRoot;
                groupParsedSuccessfully = true;
            }

            groupElementFound = true;
        } else if (m_xml.name().compare("DeletedObjects") == 0){
            parseDeletedObjects();
        } else {
            skipCurrentElement();
        }
    }

    return groupParsedSuccessfully;
}

Group* KdbxXmlReader::parseGroup()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("Group") == 0);

    auto group = new Group();
    group->setUpdateTimeinfo(false);
    QList<Group*> children;
    QList<Entry*> entries;
    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("UUID") == 0) {
            QUuid uuid = readUuid();
            if (uuid.isNull()) {
                if (m_strictMode) {
                    raiseError(tr("Null group uuid"));
                } else {
                    group->setUuid(QUuid::createUuid());
                }
            } else {
                group->setUuid(uuid);
            }
            continue;
        }
        if (m_xml.name().compare("Name") == 0) {
            group->setName(readString());
            continue;
        }
        if (m_xml.name().compare("Notes") == 0) {
            group->setNotes(readString());
            continue;
        }
        if (m_xml.name().compare("Tags") == 0) {
            group->setTags(readString());
            continue;
        }
        if (m_xml.name().compare("IconID") == 0) {
            int iconId = readNumber();
            if (iconId < 0) {
                if (m_strictMode) {
                    raiseError(tr("Invalid group icon number"));
                }
                iconId = 0;
            }

            group->setIcon(iconId);
            continue;
        }
        if (m_xml.name().compare("CustomIconUUID") == 0) {
            QUuid uuid = readUuid();
            if (!uuid.isNull()) {
                group->setIcon(uuid);
            }
            continue;
        }
        if (m_xml.name().compare("Times") == 0) {
            group->setTimeInfo(parseTimes());
            continue;
        }
        if (m_xml.name().compare("IsExpanded") == 0) {
            group->setExpanded(readBool());
            continue;
        }
        if (m_xml.name().compare("DefaultAutoTypeSequence") == 0) {
            group->setDefaultAutoTypeSequence(readString());
            continue;
        }
        if (m_xml.name().compare("EnableAutoType") == 0) {
            QString str = readString();

            if (str.compare("null", Qt::CaseInsensitive) == 0) {
                group->setAutoTypeEnabled(Group::Inherit);
            } else if (str.compare("true", Qt::CaseInsensitive) == 0) {
                group->setAutoTypeEnabled(Group::Enable);
            } else if (str.compare("false", Qt::CaseInsensitive) == 0) {
                group->setAutoTypeEnabled(Group::Disable);
            } else {
                raiseError(tr("Invalid EnableAutoType value"));
            }
            continue;
        }
        if (m_xml.name().compare("EnableSearching") == 0) {
            QString str = readString();

            if (str.compare("null", Qt::CaseInsensitive) == 0) {
                group->setSearchingEnabled(Group::Inherit);
            } else if (str.compare("true", Qt::CaseInsensitive) == 0) {
                group->setSearchingEnabled(Group::Enable);
            } else if (str.compare("false", Qt::CaseInsensitive) == 0) {
                group->setSearchingEnabled(Group::Disable);
            } else {
                raiseError(tr("Invalid EnableSearching value"));
            }
            continue;
        }
        if (m_xml.name().compare("LastTopVisibleEntry") == 0) {
            group->setLastTopVisibleEntry(getEntry(readUuid()));
            continue;
        }
        if (m_xml.name().compare("Group") == 0) {
            Group* newGroup = parseGroup();
            if (newGroup) {
                children.append(newGroup);
            }
            continue;
        }
        if (m_xml.name().compare("Entry") == 0) {
            Entry* newEntry = parseEntry(false);
            if (newEntry) {
                entries.append(newEntry);
            }
            continue;
        }
        if (m_xml.name().compare("CustomData") == 0) {
            parseCustomData(group->customData());
            continue;
        }
        if (m_xml.name().compare("PreviousParentGroup") == 0) {
            group->setPreviousParentGroupUuid(readUuid());
            continue;
        }

        skipCurrentElement();
    }

    if (group->uuid().isNull() && !m_strictMode) {
        group->setUuid(QUuid::createUuid());
    }

    if (!group->uuid().isNull()) {
        Group* tmpGroup = group;
        group = getGroup(tmpGroup->uuid());
        group->copyDataFrom(tmpGroup);
        group->setUpdateTimeinfo(false);
        delete tmpGroup;
    } else if (!hasError()) {
        raiseError(tr("No group uuid found"));
    }

    for (Group* child : asConst(children)) {
        child->setParent(group, -1, false);
    }

    for (Entry* entry : asConst(entries)) {
        entry->setGroup(group, false);
    }

    return group;
}

void KdbxXmlReader::parseDeletedObjects()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("DeletedObjects") == 0);

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("DeletedObject") == 0) {
            parseDeletedObject();
        } else {
            skipCurrentElement();
        }
    }
}

void KdbxXmlReader::parseDeletedObject()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("DeletedObject") == 0);

    DeletedObject delObj{{}, {}};

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("UUID") == 0) {
            QUuid uuid = readUuid();
            if (uuid.isNull()) {
                if (m_strictMode) {
                    raiseError(tr("Null DeleteObject uuid"));
                    return;
                }
                continue;
            }
            delObj.uuid = uuid;
            continue;
        }
        if (m_xml.name().compare("DeletionTime") == 0) {
            delObj.deletionTime = readDateTime();
            continue;
        }
        skipCurrentElement();
    }

    if (!delObj.uuid.isNull() && !delObj.deletionTime.isNull()) {
        m_db->addDeletedObject(delObj);
        return;
    }

    if (m_strictMode) {
        raiseError(tr("Missing DeletedObject uuid or time"));
    }
}

Entry* KdbxXmlReader::parseEntry(bool history)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("Entry") == 0);

    auto entry = new Entry();
    entry->setUpdateTimeinfo(false);
    QList<Entry*> historyItems;
    QList<StringPair> binaryRefs;

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("UUID") == 0) {
            QUuid uuid = readUuid();
            if (uuid.isNull()) {
                if (m_strictMode) {
                    raiseError(tr("Null entry uuid"));
                } else {
                    entry->setUuid(QUuid::createUuid());
                }
            } else {
                entry->setUuid(uuid);
            }
            continue;
        }
        if (m_xml.name().compare("IconID") == 0) {
            int iconId = readNumber();
            if (iconId < 0) {
                if (m_strictMode) {
                    raiseError(tr("Invalid entry icon number"));
                }
                iconId = 0;
            }
            entry->setIcon(iconId);
            continue;
        }
        if (m_xml.name().compare("CustomIconUUID") == 0) {
            QUuid uuid = readUuid();
            if (!uuid.isNull()) {
                entry->setIcon(uuid);
            }
            continue;
        }
        if (m_xml.name().compare("ForegroundColor") == 0) {
            entry->setForegroundColor(readColor());
            continue;
        }
        if (m_xml.name().compare("BackgroundColor") == 0) {
            entry->setBackgroundColor(readColor());
            continue;
        }
        if (m_xml.name().compare("OverrideURL") == 0) {
            entry->setOverrideUrl(readString());
            continue;
        }
        if (m_xml.name().compare("Tags") == 0) {
            entry->setTags(readString());
            continue;
        }
        if (m_xml.name().compare("Times") == 0) {
            entry->setTimeInfo(parseTimes());
            continue;
        }
        if (m_xml.name().compare("String") == 0) {
            parseEntryString(entry);
            continue;
        }
        if (m_xml.name().compare("QualityCheck") == 0) {
            entry->setExcludeFromReports(!readBool());
            continue;
        }
        if (m_xml.name().compare("Binary") == 0) {
            QPair<QString, QString> ref = parseEntryBinary(entry);
            if (!ref.first.isEmpty() && !ref.second.isEmpty()) {
                binaryRefs.append(ref);
            }
            continue;
        }
        if (m_xml.name().compare("AutoType") == 0) {
            parseAutoType(entry);
            continue;
        }
        if (m_xml.name().compare("History") == 0) {
            if (history) {
                raiseError(tr("History element in history entry"));
            } else {
                historyItems = parseEntryHistory();
            }
            continue;
        }
        if (m_xml.name().compare("CustomData") == 0) {
            parseCustomData(entry->customData());

            // Upgrade pre-KDBX-4.1 password report exclude flag
            if (entry->customData()->contains(CustomData::ExcludeFromReportsLegacy)) {
                entry->setExcludeFromReports(entry->customData()->value(CustomData::ExcludeFromReportsLegacy)
                                             == TRUE_STR);
                entry->customData()->remove(CustomData::ExcludeFromReportsLegacy);
            }
            continue;
        }
        if (m_xml.name().compare("PreviousParentGroup") == 0) {
            entry->setPreviousParentGroupUuid(readUuid());
            continue;
        }
        skipCurrentElement();
    }

    if (entry->uuid().isNull() && !m_strictMode) {
        entry->setUuid(QUuid::createUuid());
    }

    if (!entry->uuid().isNull()) {
        if (history) {
            entry->setUpdateTimeinfo(false);
        } else {
            Entry* tmpEntry = entry;

            entry = getEntry(tmpEntry->uuid());
            entry->copyDataFrom(tmpEntry);
            entry->setUpdateTimeinfo(false);

            delete tmpEntry;
        }
    } else if (!hasError()) {
        raiseError(tr("No entry uuid found"));
    }

    for (Entry* historyItem : asConst(historyItems)) {
        if (historyItem->uuid() != entry->uuid()) {
            if (m_strictMode) {
                raiseError(tr("History element with different uuid"));
            } else {
                historyItem->setUuid(entry->uuid());
            }
        }
        entry->addHistoryItem(historyItem);
    }

    for (const StringPair& ref : asConst(binaryRefs)) {
        m_binaryMap.insert(ref.first, qMakePair(entry, ref.second));
    }

    return entry;
}

void KdbxXmlReader::parseEntryString(Entry* entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("String") == 0);

    QString key;
    QString value;
    bool protect = false;
    bool keySet = false;
    bool valueSet = false;

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Key") == 0) {
            key = readString();
            keySet = true;
            continue;
        }

        if (m_xml.name().compare("Value") == 0) {
            QXmlStreamAttributes attr = m_xml.attributes();
            bool isProtected;
            bool protectInMemory;
            value = readString(isProtected, protectInMemory);
            protect = isProtected || protectInMemory;
            valueSet = true;
            continue;
        }

        skipCurrentElement();
    }

    if (keySet && valueSet) {
        // the default attributes are always there so additionally check if it's empty
        if (entry->attributes()->hasKey(key) && !entry->attributes()->value(key).isEmpty()) {
            raiseError(tr("Duplicate custom attribute found"));
            return;
        }
        entry->attributes()->set(key, value, protect);
        return;
    }

    raiseError(tr("Entry string key or value missing"));
}

QPair<QString, QString> KdbxXmlReader::parseEntryBinary(Entry* entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("Binary") == 0);

    QPair<QString, QString> poolRef;

    QString key;
    QByteArray value;
    bool keySet = false;
    bool valueSet = false;

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Key") == 0) {
            key = readString();
            keySet = true;
            continue;
        }
        if (m_xml.name().compare("Value") == 0) {
            QXmlStreamAttributes attr = m_xml.attributes();

            if (attr.hasAttribute("Ref")) {
                poolRef = qMakePair(attr.value("Ref").toString(), key);
                m_xml.skipCurrentElement();
            } else {
                // format compatibility
                value = readBinary();
            }

            valueSet = true;
            continue;
        }
        skipCurrentElement();
    }

    if (keySet && valueSet) {
        if (entry->attachments()->hasKey(key) && entry->attachments()->value(key) != value) {
            // NOTE: This only impacts KDBX 3.x databases
            // Prepend a random string to the key to make it unique and prevent data loss
            key = key.prepend(QUuid::createUuid().toString().mid(1, 8) + "_");
            qWarning("Duplicate attachment name found, renamed to: %s", qPrintable(key));
        }
        entry->attachments()->set(key, value);
    } else {
        raiseError(tr("Entry binary key or value missing"));
    }

    return poolRef;
}

void KdbxXmlReader::parseAutoType(Entry* entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("AutoType") == 0);

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Enabled") == 0) {
            entry->setAutoTypeEnabled(readBool());
        } else if (m_xml.name().compare("DataTransferObfuscation") == 0) {
            entry->setAutoTypeObfuscation(readNumber());
        } else if (m_xml.name().compare("DefaultSequence") == 0) {
            entry->setDefaultAutoTypeSequence(readString());
        } else if (m_xml.name().compare("Association") == 0) {
            parseAutoTypeAssoc(entry);
        } else {
            skipCurrentElement();
        }
    }
}

void KdbxXmlReader::parseAutoTypeAssoc(Entry* entry)
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("Association") == 0);

    AutoTypeAssociations::Association assoc;
    bool windowSet = false;
    bool sequenceSet = false;

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Window") == 0) {
            assoc.window = readString();
            windowSet = true;
        } else if (m_xml.name().compare("KeystrokeSequence") == 0) {
            assoc.sequence = readString();
            sequenceSet = true;
        } else {
            skipCurrentElement();
        }
    }

    if (windowSet && sequenceSet) {
        entry->autoTypeAssociations()->add(assoc);
        return;
    }
    raiseError(tr("Auto-type association window or sequence missing"));
}

QList<Entry*> KdbxXmlReader::parseEntryHistory()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("History") == 0);

    QList<Entry*> historyItems;

    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("Entry") == 0) {
            historyItems.append(parseEntry(true));
        } else {
            skipCurrentElement();
        }
    }

    return historyItems;
}

TimeInfo KdbxXmlReader::parseTimes()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name().compare("Times") == 0);

    TimeInfo timeInfo;
    while (!m_xml.hasError() && m_xml.readNextStartElement()) {
        if (m_xml.name().compare("LastModificationTime") == 0) {
            timeInfo.setLastModificationTime(readDateTime());
        } else if (m_xml.name().compare("CreationTime") == 0) {
            timeInfo.setCreationTime(readDateTime());
        } else if (m_xml.name().compare("LastAccessTime") == 0) {
            timeInfo.setLastAccessTime(readDateTime());
        } else if (m_xml.name().compare("ExpiryTime") == 0) {
            timeInfo.setExpiryTime(readDateTime());
        } else if (m_xml.name().compare("Expires") == 0) {
            timeInfo.setExpires(readBool());
        } else if (m_xml.name().compare("UsageCount") == 0) {
            timeInfo.setUsageCount(readNumber());
        } else if (m_xml.name().compare("LocationChanged") == 0) {
            timeInfo.setLocationChanged(readDateTime());
        } else {
            skipCurrentElement();
        }
    }

    return timeInfo;
}

QString KdbxXmlReader::readString()
{
    bool isProtected;
    bool protectInMemory;

    return readString(isProtected, protectInMemory);
}

QString KdbxXmlReader::readString(bool& isProtected, bool& protectInMemory)
{
    QXmlStreamAttributes attr = m_xml.attributes();
    isProtected = isTrueValue(attr.value("Protected"));
    protectInMemory = isTrueValue(attr.value("ProtectInMemory"));
    QString value = m_xml.readElementText();

    if (isProtected && !value.isEmpty()) {
        QByteArray ciphertext = QByteArray::fromBase64(value.toLatin1());
        bool ok;
        QByteArray plaintext = m_randomStream->process(ciphertext, &ok);
        if (!ok) {
            value.clear();
            raiseError(m_randomStream->errorString());
            return value;
        }

        value = QString::fromUtf8(plaintext);
    }

    return value;
}

bool KdbxXmlReader::readBool()
{
    QString str = readString();

    if (str.compare("true", Qt::CaseInsensitive) == 0) {
        return true;
    }
    if (str.compare("false", Qt::CaseInsensitive) == 0) {
        return false;
    }
    if (str.length() == 0) {
        return false;
    }
    raiseError(tr("Invalid bool value"));
    return false;
}

QDateTime KdbxXmlReader::readDateTime()
{
    QString str = readString();
    if (Tools::isBase64(str.toLatin1())) {
        QByteArray secsBytes = QByteArray::fromBase64(str.toUtf8()).leftJustified(8, '\0', true).left(8);
        qint64 secs = Endian::bytesToSizedInt<quint64>(secsBytes, KeePass2::BYTEORDER);
        return QDateTime(QDate(1, 1, 1), QTime(0, 0, 0, 0), Qt::UTC).addSecs(secs);
    }

    QDateTime dt = Clock::parse(str, Qt::ISODate);
    if (dt.isValid()) {
        return dt;
    }

    if (m_strictMode) {
        raiseError(tr("Invalid date time value"));
    }

    return Clock::currentDateTimeUtc();
}

QString KdbxXmlReader::readColor()
{
    QString colorStr = readString();

    if (colorStr.isEmpty()) {
        return colorStr;
    }

    if (colorStr.length() != 7 || colorStr[0] != '#') {
        if (m_strictMode) {
            raiseError(tr("Invalid color value"));
        }
        return colorStr;
    }

    for (int i = 0; i <= 2; ++i) {
        QString rgbPartStr = colorStr.mid(1 + 2 * i, 2);
        bool ok;
        int rgbPart = rgbPartStr.toInt(&ok, 16);
        if (!ok || rgbPart > 255) {
            if (m_strictMode) {
                raiseError(tr("Invalid color rgb part"));
            }
            return colorStr;
        }
    }

    return colorStr;
}

int KdbxXmlReader::readNumber()
{
    bool ok;
    int result = readString().toInt(&ok);
    if (!ok) {
        raiseError(tr("Invalid number value"));
    }
    return result;
}

QUuid KdbxXmlReader::readUuid()
{
    QByteArray uuidBin = readBinary();
    if (uuidBin.isEmpty()) {
        return {};
    }
    if (uuidBin.length() != UUID_LENGTH) {
        if (m_strictMode) {
            raiseError(tr("Invalid uuid value"));
        }
        return {};
    }
    return QUuid::fromRfc4122(uuidBin);
}

QByteArray KdbxXmlReader::readBinary()
{
    QXmlStreamAttributes attr = m_xml.attributes();
    bool isProtected = isTrueValue(attr.value("Protected"));
    QString value = m_xml.readElementText();
    QByteArray data = QByteArray::fromBase64(value.toLatin1());

    if (isProtected && !data.isEmpty()) {
        bool ok;
        QByteArray plaintext = m_randomStream->process(data, &ok);
        if (!ok) {
            data.clear();
            raiseError(m_randomStream->errorString());
            return data;
        }

        data = plaintext;
    }

    return data;
}

QByteArray KdbxXmlReader::readCompressedBinary()
{
    QByteArray rawData = readBinary();

    QBuffer buffer(&rawData);
    buffer.open(QIODevice::ReadOnly);

    QtIOCompressor compressor(&buffer);
    compressor.setStreamFormat(QtIOCompressor::GzipFormat);
    compressor.open(QIODevice::ReadOnly);

    QByteArray result;
    if (!Tools::readAllFromDevice(&compressor, result)) {
        //: Translator meant is a binary data inside an entry
        raiseError(tr("Unable to decompress binary"));
    }
    return result;
}

Group* KdbxXmlReader::getGroup(const QUuid& uuid)
{
    if (uuid.isNull()) {
        return nullptr;
    }

    if (m_groups.contains(uuid)) {
        return m_groups.value(uuid);
    }

    auto group = new Group();
    group->setUpdateTimeinfo(false);
    group->setUuid(uuid);
    group->setParent(m_tmpParent.data());
    m_groups.insert(uuid, group);
    return group;
}

Entry* KdbxXmlReader::getEntry(const QUuid& uuid)
{
    if (uuid.isNull()) {
        return nullptr;
    }

    if (m_entries.contains(uuid)) {
        return m_entries.value(uuid);
    }

    auto entry = new Entry();
    entry->setUpdateTimeinfo(false);
    entry->setUuid(uuid);
    entry->setGroup(m_tmpParent.data());
    m_entries.insert(uuid, entry);
    return entry;
}

void KdbxXmlReader::skipCurrentElement()
{
    qWarning("KdbxXmlReader::skipCurrentElement: skip element \"%s\"", qPrintable(m_xml.name().toString()));
    m_xml.skipCurrentElement();
}
