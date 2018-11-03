/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_DATABASE_H
#define KEEPASSX_DATABASE_H

#include <QDateTime>
#include <QHash>
#include <QObject>

#include "crypto/kdf/Kdf.h"
#include "format/KeePass2.h"
#include "keys/CompositeKey.h"
#include "crypto/kdf/AesKdf.h"

class Entry;
enum class EntryReferenceType;
class Group;
class Metadata;
class QTimer;
class QIODevice;

struct DeletedObject
{
    QUuid uuid;
    QDateTime deletionTime;
    bool operator==(const DeletedObject& other) const
    {
        return uuid == other.uuid && deletionTime == other.deletionTime;
    }
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

    Database();
    explicit Database(const QString& filePath);
    ~Database() override;

    QUuid uuid() const;

    bool open(QSharedPointer<const CompositeKey> key, bool readOnly = false, QString* error = nullptr);
    bool open(const QString& filePath, QSharedPointer<const CompositeKey> key, bool readOnly = false, QString* error = nullptr);
    bool save(bool atomic = true, bool backup = false, QString* error = nullptr);
    bool save(const QString& filePath, bool atomic = true, bool backup = false, QString* error = nullptr);

    QString filePath() const;
    void setFilePath(const QString& filePath);
    bool isReadOnly() const;
    void setReadOnly(bool readOnly);

    Group* rootGroup();
    const Group* rootGroup() const;
    void setRootGroup(Group* group);

    Metadata* metadata();
    const Metadata* metadata() const;
    QVariantMap& publicCustomData();
    const QVariantMap& publicCustomData() const;
    void setPublicCustomData(const QVariantMap& customData);
    QList<DeletedObject> deletedObjects();
    const QList<DeletedObject>& deletedObjects() const;
    void addDeletedObject(const DeletedObject& delObj);
    void addDeletedObject(const QUuid& uuid);
    bool containsDeletedObject(const QUuid& uuid) const;
    bool containsDeletedObject(const DeletedObject& uuid) const;
    void setDeletedObjects(const QList<DeletedObject>& delObjs);

    bool hasKey() const;
    bool verifyKey(const QSharedPointer<CompositeKey>& key) const;
    const QUuid& cipher() const;
    void setCipher(const QUuid& cipher);
    Database::CompressionAlgorithm compressionAlgo() const;
    void setCompressionAlgo(Database::CompressionAlgorithm algo);
    QSharedPointer<Kdf> kdf() const;
    void setKdf(QSharedPointer<Kdf> kdf);
    bool setKey(const QSharedPointer<const CompositeKey>& key, bool updateChangedTime = true, bool updateTransformSalt = false);
    bool changeKdf(const QSharedPointer<Kdf>& kdf);
    QByteArray transformedMasterKey() const;
    QSharedPointer<const CompositeKey> key() const;
    QByteArray challengeResponseKey() const;
    bool challengeMasterSeed(const QByteArray& masterSeed);

    void recycleEntry(Entry* entry);
    void recycleGroup(Group* group);
    void emptyRecycleBin();

    void setEmitModified(bool value);
    void markAsModified();

    Group* resolveGroup(const QUuid& uuid);
    Entry* resolveEntry(const QUuid& uuid);
    Entry* resolveEntry(const QString& text, EntryReferenceType referenceType);

    static Database* databaseByUuid(const QUuid& uuid);;
    static Database* databaseByFilePath(const QString& filePath);;
    static Database* unlockFromStdin(QString databaseFilename, QString keyFilename = {},
        FILE* outputDescriptor = stdout, FILE* errorDescriptor = stderr);

signals:
    void filePathChanged(const QString& oldPath, const QString& newPath);
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
    struct DatabaseData
    {
        QString filePath;
        bool isReadOnly = false;
        QUuid cipher = KeePass2::CIPHER_AES256;
        CompressionAlgorithm compressionAlgo = CompressionGZip;
        QByteArray transformedMasterKey;
        QSharedPointer<Kdf> kdf = QSharedPointer<AesKdf>::create(true);
        QSharedPointer<const CompositeKey> key;
        bool hasKey = false;
        QByteArray masterSeed;
        QByteArray challengeResponseKey;
        QVariantMap publicCustomData;

        DatabaseData()
        {
            kdf->randomizeSeed();
        }
    };

    Entry* findEntryRecursive(const QUuid& uuid, Group* group);
    Entry* findEntryRecursive(const QString& text, EntryReferenceType referenceType, Group* group);
    Group* findGroupRecursive(const QUuid& uuid, Group* group);

    void createRecycleBin();

    bool writeDatabase(QIODevice* device, QString* error = nullptr);
    bool backupDatabase(const QString& filePath);

    Metadata* const m_metadata;
    DatabaseData m_data;
    Group* m_rootGroup;
    QList<DeletedObject> m_deletedObjects;
    QTimer* m_timer;
    bool m_emitModified;

    QUuid m_uuid;
    static QHash<QUuid, QPointer<Database>> s_uuidMap;
    static QHash<QString, QPointer<Database>> s_filePathMap;
};

#endif // KEEPASSX_DATABASE_H
