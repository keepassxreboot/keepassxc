/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "Database.h"

#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QTimer>
#include <QXmlStreamReader>

#include "cli/Utils.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/Random.h"
#include "format/KeePass2.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keys/PasswordKey.h"
#include "keys/FileKey.h"
#include "keys/CompositeKey.h"

QHash<Uuid, Database*> Database::m_uuidMap;

Database::Database()
    : m_metadata(new Metadata(this))
    , m_timer(new QTimer(this))
    , m_emitModified(false)
    , m_uuid(Uuid::random())
{
    m_data.cipher = KeePass2::CIPHER_AES;
    m_data.compressionAlgo = CompressionGZip;
    m_data.transformRounds = 100000;
    m_data.hasKey = false;

    setRootGroup(new Group());
    rootGroup()->setUuid(Uuid::random());
    m_timer->setSingleShot(true);

    m_uuidMap.insert(m_uuid, this);

    connect(m_metadata, SIGNAL(modified()), this, SIGNAL(modifiedImmediate()));
    connect(m_metadata, SIGNAL(nameTextChanged()), this, SIGNAL(nameTextChanged()));
    connect(this, SIGNAL(modifiedImmediate()), this, SLOT(startModifiedTimer()));
    connect(m_timer, SIGNAL(timeout()), SIGNAL(modified()));
}

Database::~Database()
{
    m_uuidMap.remove(m_uuid);
}

Group* Database::rootGroup()
{
    return m_rootGroup;
}

const Group* Database::rootGroup() const
{
    return m_rootGroup;
}

void Database::setRootGroup(Group* group)
{
    Q_ASSERT(group);

    m_rootGroup = group;
    m_rootGroup->setParent(this);
}

Metadata* Database::metadata()
{
    return m_metadata;
}

const Metadata* Database::metadata() const
{
    return m_metadata;
}

Entry* Database::resolveEntry(const Uuid& uuid)
{
    return recFindEntry(uuid, m_rootGroup);
}

Entry *Database::resolveEntry(const QString &text, EntryReferenceType referenceType)
{
    return recFindEntry(text, referenceType, m_rootGroup);
}

Entry* Database::recFindEntry(const Uuid& uuid, Group* group)
{
    const QList<Entry*> entryList = group->entries();
    for (Entry* entry : entryList) {
        if (entry->uuid() == uuid) {
            return entry;
        }
    }

    const QList<Group*> children = group->children();
    for (Group* child : children) {
        Entry* result = recFindEntry(uuid, child);
        if (result) {
            return result;
        }
    }

    return nullptr;
}

Entry *Database::recFindEntry(const QString &text, EntryReferenceType referenceType, Group *group)
{
    Q_ASSERT_X(referenceType != EntryReferenceType::Unknown, "Database::recFindEntry",
               "Can't search entry with \"referenceType\" parameter equal to \"Unknown\"");

    bool found = false;
    const QList<Entry*> entryList = group->entries();
    for (Entry* entry : entryList) {
        switch (referenceType) {
        case EntryReferenceType::Unknown:
            return nullptr;
        case EntryReferenceType::Title:
            found = entry->title() == text;
            break;
        case EntryReferenceType::UserName:
            found = entry->username() == text;
            break;
        case EntryReferenceType::Password:
            found = entry->password() == text;
            break;
        case EntryReferenceType::Url:
            found = entry->url() == text;
            break;
        case EntryReferenceType::Notes:
            found = entry->notes() == text;
            break;
        case EntryReferenceType::Uuid:
            found = entry->uuid().toHex() == text;
            break;
        case EntryReferenceType::CustomAttributes:
            found = entry->attributes()->containsValue(text);
            break;
        }

        if (found) {
            return entry;
        }
    }

    const QList<Group*> children = group->children();
    for (Group* child : children) {
        Entry* result = recFindEntry(text, referenceType, child);
        if (result) {
            return result;
        }
    }

    return nullptr;
}

Group* Database::resolveGroup(const Uuid& uuid)
{
    return recFindGroup(uuid, m_rootGroup);
}

Group* Database::recFindGroup(const Uuid& uuid, Group* group)
{
    if (group->uuid() == uuid) {
        return group;
    }

    const QList<Group*> children = group->children();
    for (Group* child : children) {
        Group* result = recFindGroup(uuid, child);
        if (result) {
            return result;
        }
    }

    return nullptr;
}

QList<DeletedObject> Database::deletedObjects()
{
    return m_deletedObjects;
}

void Database::addDeletedObject(const DeletedObject& delObj)
{
    Q_ASSERT(delObj.deletionTime.timeSpec() == Qt::UTC);
    m_deletedObjects.append(delObj);
}

void Database::addDeletedObject(const Uuid& uuid)
{
    DeletedObject delObj;
    delObj.deletionTime = QDateTime::currentDateTimeUtc();
    delObj.uuid = uuid;

    addDeletedObject(delObj);
}

Uuid Database::cipher() const
{
    return m_data.cipher;
}

Database::CompressionAlgorithm Database::compressionAlgo() const
{
    return m_data.compressionAlgo;
}

QByteArray Database::transformSeed() const
{
    return m_data.transformSeed;
}

quint64 Database::transformRounds() const
{
    return m_data.transformRounds;
}

QByteArray Database::transformedMasterKey() const
{
    return m_data.transformedMasterKey;
}

QByteArray Database::challengeResponseKey() const
{
    return m_data.challengeResponseKey;
}

bool Database::challengeMasterSeed(const QByteArray& masterSeed)
{
    m_data.masterSeed = masterSeed;
    return m_data.key.challenge(masterSeed, m_data.challengeResponseKey);
}

void Database::setCipher(const Uuid& cipher)
{
    Q_ASSERT(!cipher.isNull());

    m_data.cipher = cipher;
}

void Database::setCompressionAlgo(Database::CompressionAlgorithm algo)
{
    Q_ASSERT(static_cast<quint32>(algo) <= CompressionAlgorithmMax);

    m_data.compressionAlgo = algo;
}

bool Database::setTransformRounds(quint64 rounds)
{
    if (m_data.transformRounds != rounds) {
        quint64 oldRounds = m_data.transformRounds;

        m_data.transformRounds = rounds;

        if (m_data.hasKey) {
            if (!setKey(m_data.key)) {
                m_data.transformRounds = oldRounds;
                return false;
            }
        }
    }

    return true;
}

bool Database::setKey(const CompositeKey& key, const QByteArray& transformSeed, bool updateChangedTime)
{
    bool ok;
    QString errorString;

    QByteArray transformedMasterKey = key.transform(transformSeed, transformRounds(), &ok, &errorString);
    if (!ok) {
        return false;
    }

    m_data.key = key;
    m_data.transformSeed = transformSeed;
    m_data.transformedMasterKey = transformedMasterKey;
    m_data.hasKey = true;
    if (updateChangedTime) {
        m_metadata->setMasterKeyChanged(QDateTime::currentDateTimeUtc());
    }
    emit modifiedImmediate();

    return true;
}

bool Database::setKey(const CompositeKey& key)
{
    return setKey(key, randomGen()->randomArray(32));
}

bool Database::hasKey() const
{
    return m_data.hasKey;
}

bool Database::transformKeyWithSeed(const QByteArray& transformSeed)
{
    Q_ASSERT(hasKey());

    bool ok;
    QString errorString;

    QByteArray transformedMasterKey =
            m_data.key.transform(transformSeed, transformRounds(), &ok, &errorString);
    if (!ok) {
        return false;
    }

    m_data.transformSeed = transformSeed;
    m_data.transformedMasterKey = transformedMasterKey;

    return true;
}

bool Database::verifyKey(const CompositeKey& key) const
{
    Q_ASSERT(hasKey());

    if (!m_data.challengeResponseKey.isEmpty()) {
        QByteArray result;

        if (!key.challenge(m_data.masterSeed, result)) {
            // challenge failed, (YubiKey?) removed?
            return false;
        }

        if (m_data.challengeResponseKey != result) {
            // wrong response from challenged device(s)
            return false;
        }
    }

    return (m_data.key.rawKey() == key.rawKey());
}

void Database::createRecycleBin()
{
    Group* recycleBin = Group::createRecycleBin();
    recycleBin->setParent(rootGroup());
    m_metadata->setRecycleBin(recycleBin);
}

void Database::recycleEntry(Entry* entry)
{
    if (m_metadata->recycleBinEnabled()) {
        if (!m_metadata->recycleBin()) {
            createRecycleBin();
        }
        entry->setGroup(metadata()->recycleBin());
    } else {
        delete entry;
    }
}

void Database::recycleGroup(Group* group)
{
    if (m_metadata->recycleBinEnabled()) {
        if (!m_metadata->recycleBin()) {
            createRecycleBin();
        }
        group->setParent(metadata()->recycleBin());
    } else {
        delete group;
    }
}

void Database::emptyRecycleBin()
{
    if (m_metadata->recycleBinEnabled() && m_metadata->recycleBin()) {
        // destroying direct entries of the recycle bin
        QList<Entry*> subEntries = m_metadata->recycleBin()->entries();
        for (Entry* entry : subEntries) {
            delete entry;
        }
        // destroying direct subgroups of the recycle bin
        QList<Group*> subGroups = m_metadata->recycleBin()->children();
        for (Group* group : subGroups) {
            delete group;
        }
    }
}

void Database::merge(const Database* other)
{
    m_rootGroup->merge(other->rootGroup());

    for (Uuid customIconId : other->metadata()->customIcons().keys()) {
        QImage customIcon = other->metadata()->customIcon(customIconId);
        if (!this->metadata()->containsCustomIcon(customIconId)) {
            qDebug("Adding custom icon %s to database.", qPrintable(customIconId.toHex()));
            this->metadata()->addCustomIcon(customIconId, customIcon);
        }
    }

    emit modified();
}

void Database::setEmitModified(bool value)
{
    if (m_emitModified && !value) {
        m_timer->stop();
    }

    m_emitModified = value;
}

void Database::copyAttributesFrom(const Database* other)
{
    m_data = other->m_data;
    m_metadata->copyAttributesFrom(other->m_metadata);
}

Uuid Database::uuid()
{
    return m_uuid;
}

Database* Database::databaseByUuid(const Uuid& uuid)
{
    return m_uuidMap.value(uuid, 0);
}

void Database::startModifiedTimer()
{
    if (!m_emitModified) {
        return;
    }

    if (m_timer->isActive()) {
        m_timer->stop();
    }
    m_timer->start(150);
}

const CompositeKey& Database::key() const
{
    return m_data.key;
}

Database* Database::openDatabaseFile(QString fileName, CompositeKey key)
{

    QFile dbFile(fileName);
    if (!dbFile.exists()) {
        qCritical("File %s does not exist.", qPrintable(fileName));
        return nullptr;
    }
    if (!dbFile.open(QIODevice::ReadOnly)) {
        qCritical("Unable to open file %s.", qPrintable(fileName));
        return nullptr;
    }

    KeePass2Reader reader;
    Database* db = reader.readDatabase(&dbFile, key);

    if (reader.hasError()) {
        qCritical("Error while parsing the database: %s", qPrintable(reader.errorString()));
        return nullptr;
    }

    return db;
}

Database* Database::unlockFromStdin(QString databaseFilename, QString keyFilename)
{
    CompositeKey compositeKey;
    QTextStream outputTextStream(stdout);
    QTextStream errorTextStream(stderr);

    outputTextStream << QObject::tr("Insert password to unlock %1: ").arg(databaseFilename);
    outputTextStream.flush();

    QString line = Utils::getPassword();
    PasswordKey passwordKey;
    passwordKey.setPassword(line);
    compositeKey.addKey(passwordKey);

    if (!keyFilename.isEmpty()) {
        FileKey fileKey;
        QString errorMessage;
        if (!fileKey.load(keyFilename, &errorMessage)) {
            errorTextStream << QObject::tr("Failed to load key file %1 : %2").arg(keyFilename, errorMessage);
            errorTextStream << endl;
            return nullptr;
        }
        compositeKey.addKey(fileKey);
    }

    return Database::openDatabaseFile(databaseFilename, compositeKey);
}

QString Database::saveToFile(QString filePath)
{
    KeePass2Writer writer;
    QSaveFile saveFile(filePath);
    if (saveFile.open(QIODevice::WriteOnly)) {

        // write the database to the file
        writer.writeDatabase(&saveFile, this);

        if (writer.hasError()) {
            return writer.errorString();
        }

        if (saveFile.commit()) {
            // successfully saved database file
            return QString();
        } else {
            return saveFile.errorString();
        }
    } else {
        return saveFile.errorString();
    }
}
