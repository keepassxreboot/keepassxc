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

#include <QtCore/QFile>
#include <QtCore/QXmlStreamReader>

#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/Random.h"
#include "format/KeePass2.h"

Database::Database()
{
    m_hasKey = false;
    m_metadata = new Metadata(this);
    setRootGroup(new Group());
    rootGroup()->setUuid(Uuid::random());

    m_cipher = KeePass2::CIPHER_AES;
    m_compressionAlgo = CompressionGZip;
    m_transformRounds = 50000;

    connect(m_metadata, SIGNAL(modified()), this, SIGNAL(modified()));
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

    return 0;
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

    return 0;
}

QList<DeletedObject> Database::deletedObjects()
{
    return m_deletedObjects;
}

void Database::addDeletedObject(const DeletedObject& delObj)
{
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
    return m_cipher;
}

Database::CompressionAlgorithm Database::compressionAlgo() const
{
    return m_compressionAlgo;
}

QByteArray Database::transformSeed() const
{
    return m_transformSeed;
}

quint64 Database::transformRounds() const
{
    return m_transformRounds;
}

QByteArray Database::transformedMasterKey() const
{
    return m_transformedMasterKey;
}

void Database::setCipher(const Uuid& cipher)
{
    Q_ASSERT(!cipher.isNull());

    m_cipher = cipher;
}

void Database::setCompressionAlgo(Database::CompressionAlgorithm algo)
{
    Q_ASSERT(static_cast<quint32>(algo) <= CompressionAlgorithmMax);

    m_compressionAlgo = algo;
}

void Database::setTransformRounds(quint64 rounds)
{
    m_transformRounds = rounds;
}

void Database::setKey(const CompositeKey& key, const QByteArray& transformSeed, bool updateChangedTime)
{
    m_key = key;
    m_transformSeed = transformSeed;
    m_transformedMasterKey = key.transform(transformSeed, transformRounds());
    m_hasKey = true;
    if (updateChangedTime) {
        m_metadata->setMasterKeyChanged(QDateTime::currentDateTimeUtc());
    }
    Q_EMIT modified();
}

void Database::setKey(const CompositeKey& key)
{
    setKey(key, Random::randomArray(32));
}

void Database::updateKey(quint64 rounds)
{
    if (m_transformRounds != rounds) {
        m_transformRounds = rounds;
        m_transformedMasterKey = m_key.transform(m_transformSeed, transformRounds());
        m_metadata->setMasterKeyChanged(QDateTime::currentDateTimeUtc());
        Q_EMIT modified();
    }
}

bool Database::hasKey()
{
    return m_hasKey;
}

void Database::createRecycleBin()
{
    Group* recycleBin = new Group();
    recycleBin->setUuid(Uuid::random());
    recycleBin->setName(tr("Recycle Bin"));
    recycleBin->setIcon(43);
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
