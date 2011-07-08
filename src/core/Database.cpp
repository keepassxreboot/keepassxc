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
    m_metadata = new Metadata(this);
    m_rootGroup = 0;

    m_cipher = KeePass2::CIPHER_AES;
    m_compressionAlgo = CompressionGZip;
    m_transformRounds = 50000;
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
    if (group != 0) {
        group->setParent(this);
    }

    m_rootGroup = group;
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
        if (entry->uuid() == uuid)
            return entry;
    }

    Q_FOREACH (Group* child, group->children()) {
        Entry* result = recFindEntry(uuid, child);
        if (result)
            return result;
    }

    return 0;
}

Group* Database::resolveGroup(const Uuid& uuid)
{
    return recFindGroup(uuid, m_rootGroup);
}

Group* Database::recFindGroup(const Uuid& uuid, Group* group)
{
    if (group->uuid() == uuid)
        return group;

    Q_FOREACH (Group* child, group->children()) {
        Group* result = recFindGroup(uuid, child);
        if (result)
            return result;
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

void Database::setKey(const CompositeKey& key, const QByteArray& transformSeed)
{
    m_transformSeed = transformSeed;
    m_transformedMasterKey = key.transform(transformSeed, transformRounds());
}

void Database::setKey(const CompositeKey& key)
{
    setKey(key, Random::randomArray(32));
}
