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

#ifndef KEEPASSX_DATABASE_H
#define KEEPASSX_DATABASE_H

#include <QDateTime>
#include <QHash>
#include <QObject>

#include "crypto/kdf/Kdf.h"
#include "core/Uuid.h"
#include "keys/CompositeKey.h"

class Entry;
enum class EntryReferenceType;
class Group;
class Metadata;
class QTimer;
class QIODevice;

struct DeletedObject
{
    Uuid uuid;
    QDateTime deletionTime;
};

Q_DECLARE_TYPEINFO(DeletedObject, Q_MOVABLE_TYPE);

class Database : public QObject
{
    Q_OBJECT

public:
    enum CompressionAlgorithm
    {
        CompressionNone = 0,
        CompressionGZip = 1
    };
    static const quint32 CompressionAlgorithmMax = CompressionGZip;

    struct DatabaseData
    {
        Uuid cipher;
        CompressionAlgorithm compressionAlgo;
        QByteArray transformedMasterKey;
        QSharedPointer<Kdf> kdf;
        CompositeKey key;
        bool hasKey;
        QByteArray masterSeed;
        QByteArray challengeResponseKey;
        QVariantMap publicCustomData;
    };

    Database();
    ~Database() override;
    Group* rootGroup();
    const Group* rootGroup() const;

    /**
     * Sets group as the root group and takes ownership of it.
     * Warning: Be careful when calling this method as it doesn't
     *          emit any notifications so e.g. models aren't updated.
     *          The caller is responsible for cleaning up the previous
                root group.
     */
    void setRootGroup(Group* group);

    Metadata* metadata();
    const Metadata* metadata() const;
    Entry* resolveEntry(const Uuid& uuid);
    Entry* resolveEntry(const QString& text, EntryReferenceType referenceType);
    Group* resolveGroup(const Uuid& uuid);
    QList<DeletedObject> deletedObjects();
    void addDeletedObject(const DeletedObject& delObj);
    void addDeletedObject(const Uuid& uuid);

    Uuid cipher() const;
    Database::CompressionAlgorithm compressionAlgo() const;
    QSharedPointer<Kdf> kdf() const;
    QByteArray transformedMasterKey() const;
    const CompositeKey& key() const;
    QByteArray challengeResponseKey() const;
    bool challengeMasterSeed(const QByteArray& masterSeed);

    void setCipher(const Uuid& cipher);
    void setCompressionAlgo(Database::CompressionAlgorithm algo);
    void setKdf(QSharedPointer<Kdf> kdf);
    bool setKey(const CompositeKey& key, bool updateChangedTime = true,
                bool updateTransformSalt = false);
    bool hasKey() const;
    bool verifyKey(const CompositeKey& key) const;
    QVariantMap& publicCustomData();
    const QVariantMap& publicCustomData() const;
    void setPublicCustomData(const QVariantMap& customData);
    void recycleEntry(Entry* entry);
    void recycleGroup(Group* group);
    void emptyRecycleBin();
    void setEmitModified(bool value);
    void merge(const Database* other);
    QString saveToFile(QString filePath, bool atomic = true, bool backup = false);

    /**
     * Returns a unique id that is only valid as long as the Database exists.
     */
    Uuid uuid();
    bool changeKdf(QSharedPointer<Kdf> kdf);

    static Database* databaseByUuid(const Uuid& uuid);
    static Database* openDatabaseFile(QString fileName, CompositeKey key);
    static Database* unlockFromStdin(QString databaseFilename, QString keyFilename = QString(""));

signals:
    void groupDataChanged(Group* group);
    void groupAboutToAdd(Group* group, int index);
    void groupAdded();
    void groupAboutToRemove(Group* group);
    void groupRemoved();
    void groupAboutToMove(Group* group, Group* toGroup, int index);
    void groupMoved();
    void nameTextChanged();
    void modified();
    void modifiedImmediate();

private slots:
    void startModifiedTimer();

private:
    Entry* findEntryRecursive(const Uuid& uuid, Group* group);
    Entry* findEntryRecursive(const QString& text, EntryReferenceType referenceType, Group* group);
    Group* findGroupRecursive(const Uuid& uuid, Group* group);

    void createRecycleBin();
    QString writeDatabase(QIODevice* device);
    bool backupDatabase(QString filePath);

    Metadata* const m_metadata;
    Group* m_rootGroup;
    QList<DeletedObject> m_deletedObjects;
    QTimer* m_timer;
    DatabaseData m_data;
    bool m_emitModified;

    Uuid m_uuid;
    static QHash<Uuid, Database*> m_uuidMap;
};

#endif // KEEPASSX_DATABASE_H
