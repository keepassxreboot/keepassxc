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

#include "Database.h"

#include <QFile>
#include <QTimer>
#include <QXmlStreamReader>

#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "crypto/Random.h"
#include "format/KeePass2.h"

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

Entry* Database::recFindEntry(const Uuid& uuid, Group* group)
{
    Q_FOREACH (Entry* entry, group->entries()) {
        if (entry->uuid() == uuid) {
            return entry;
        }
    }

    Q_FOREACH (Group* child, group->children()) {
        Entry* result = recFindEntry(uuid, child);
        if (result) {
            return result;
        }
    }

    return Q_NULLPTR;
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

    Q_FOREACH (Group* child, group->children()) {
        Group* result = recFindGroup(uuid, child);
        if (result) {
            return result;
        }
    }

    return Q_NULLPTR;
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
    delObj.deletionTime = Tools::currentDateTimeUtc();
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

void Database::setTransformRounds(quint64 rounds)
{
    if (m_data.transformRounds != rounds) {
        m_data.transformRounds = rounds;

        if (m_data.hasKey) {
            setKey(m_data.key);
        }
    }
}

void Database::setKey(const CompositeKey& key, const QByteArray& transformSeed, bool updateChangedTime)
{
    m_data.key = key;
    m_data.transformSeed = transformSeed;
    m_data.transformedMasterKey = key.transform(transformSeed, transformRounds());
    m_data.hasKey = true;
    if (updateChangedTime) {
        m_metadata->setMasterKeyChanged(Tools::currentDateTimeUtc());
    }
    Q_EMIT modifiedImmediate();
}

void Database::setKey(const CompositeKey& key)
{
    setKey(key, randomGen()->randomArray(32));
}

bool Database::hasKey() const
{
    return m_data.hasKey;
}

bool Database::verifyKey(const CompositeKey& key) const
{
    Q_ASSERT(hasKey());

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
    }
    else {
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
    }
    else {
        delete group;
     }
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
