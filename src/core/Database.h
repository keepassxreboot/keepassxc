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
#include <QPointer>

#include "config-keepassx.h"
#include "crypto/kdf/AesKdf.h"
#include "crypto/kdf/Kdf.h"
#include "format/KeePass2.h"
#include "keys/CompositeKey.h"

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

    bool open(QSharedPointer<const CompositeKey> key, QString* error = nullptr, bool readOnly = false);
    bool open(const QString& filePath,
              QSharedPointer<const CompositeKey> key,
              QString* error = nullptr,
              bool readOnly = false);
    bool save(QString* error = nullptr, bool atomic = true, bool backup = false);
    bool save(const QString& filePath, QString* error = nullptr, bool atomic = true, bool backup = false);

    bool isInitialized() const;
    void setInitialized(bool initialized);
    bool isModified() const;
    void setEmitModified(bool value);
    bool isReadOnly() const;
    void setReadOnly(bool readOnly);

    QUuid uuid() const;
    QString filePath() const;
    void setFilePath(const QString& filePath);

    Metadata* metadata();
    const Metadata* metadata() const;
    Group* rootGroup();
    const Group* rootGroup() const;
    void setRootGroup(Group* group);
    QVariantMap& publicCustomData();
    const QVariantMap& publicCustomData() const;
    void setPublicCustomData(const QVariantMap& customData);

    void recycleGroup(Group* group);
    void recycleEntry(Entry* entry);
    void emptyRecycleBin();
    QList<DeletedObject> deletedObjects();
    const QList<DeletedObject>& deletedObjects() const;
    void addDeletedObject(const DeletedObject& delObj);
    void addDeletedObject(const QUuid& uuid);
    bool containsDeletedObject(const QUuid& uuid) const;
    bool containsDeletedObject(const DeletedObject& uuid) const;
    void setDeletedObjects(const QList<DeletedObject>& delObjs);

    bool hasKey() const;
    QSharedPointer<const CompositeKey> key() const;
    bool setKey(const QSharedPointer<const CompositeKey>& key,
                bool updateChangedTime = true,
                bool updateTransformSalt = false);
    QByteArray challengeResponseKey() const;
    bool challengeMasterSeed(const QByteArray& masterSeed);
    bool verifyKey(const QSharedPointer<CompositeKey>& key) const;
    const QUuid& cipher() const;
    void setCipher(const QUuid& cipher);
    Database::CompressionAlgorithm compressionAlgorithm() const;
    void setCompressionAlgorithm(Database::CompressionAlgorithm algo);

    QSharedPointer<Kdf> kdf() const;
    void setKdf(QSharedPointer<Kdf> kdf);
    bool changeKdf(const QSharedPointer<Kdf>& kdf);
    QByteArray transformedMasterKey() const;

    static Database* databaseByUuid(const QUuid& uuid);
    static Database* databaseByFilePath(const QString& filePath);

public slots:
    void markAsModified();
    void markAsClean();

signals:
    void filePathChanged(const QString& oldPath, const QString& newPath);
    void groupDataChanged(Group* group);
    void groupAboutToAdd(Group* group, int index);
    void groupAdded();
    void groupAboutToRemove(Group* group);
    void groupRemoved();
    void groupAboutToMove(Group* group, Group* toGroup, int index);
    void groupMoved();
    void databaseModified();
    void databaseSaved();
    void databaseDiscarded();

private slots:
    void startModifiedTimer();

private:
    struct DatabaseData
    {
        QString filePath;
        bool isReadOnly = false;
        QUuid cipher = KeePass2::CIPHER_AES256;
        CompressionAlgorithm compressionAlgorithm = CompressionGZip;
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

    void createRecycleBin();

    bool writeDatabase(QIODevice* device, QString* error = nullptr);
    bool backupDatabase(const QString& filePath);

    Metadata* const m_metadata;
    DatabaseData m_data;
    Group* m_rootGroup;
    QList<DeletedObject> m_deletedObjects;
    QPointer<QTimer> m_timer;
    bool m_initialized = false;
    bool m_modified = false;
    bool m_emitModified;

    QUuid m_uuid;
    static QHash<QUuid, QPointer<Database>> s_uuidMap;
    static QHash<QString, QPointer<Database>> s_filePathMap;
};

#endif // KEEPASSX_DATABASE_H
