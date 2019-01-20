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

#include "Database.h"

#include "core/Clock.h"
#include "core/Group.h"
#include "core/Merger.h"
#include "core/Metadata.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTimer>
#include <QXmlStreamReader>

QHash<QUuid, QPointer<Database>> Database::s_uuidMap;
QHash<QString, QPointer<Database>> Database::s_filePathMap;

Database::Database()
    : m_metadata(new Metadata(this))
    , m_data()
    , m_rootGroup(nullptr)
    , m_timer(new QTimer(this))
    , m_emitModified(false)
    , m_uuid(QUuid::createUuid())
{
    setRootGroup(new Group());
    rootGroup()->setUuid(QUuid::createUuid());
    rootGroup()->setName(tr("Root", "Root group name"));
    m_timer->setSingleShot(true);

    s_uuidMap.insert(m_uuid, this);

    connect(m_metadata, SIGNAL(metadataModified()), this, SLOT(markAsModified()));
    connect(m_timer, SIGNAL(timeout()), SIGNAL(databaseModified()));

    m_modified = false;
    m_emitModified = true;
}

Database::Database(const QString& filePath)
    : Database()
{
    setFilePath(filePath);
}

Database::~Database()
{
    s_uuidMap.remove(m_uuid);

    if (m_modified) {
        emit databaseDiscarded();
    }
}

QUuid Database::uuid() const
{
    return m_uuid;
}

/**
 * Open the database from a previously specified file.
 * Unless `readOnly` is set to false, the database will be opened in
 * read-write mode and fall back to read-only if that is not possible.
 *
 * @param key composite key for unlocking the database
 * @param readOnly open in read-only mode
 * @param error error message in case of failure
 * @return true on success
 */
bool Database::open(QSharedPointer<const CompositeKey> key, QString* error, bool readOnly)
{
    Q_ASSERT(!m_data.filePath.isEmpty());
    if (m_data.filePath.isEmpty()) {
        return false;
    }
    return open(m_data.filePath, std::move(key), error, readOnly);
}

/**
 * Open the database from a file.
 * Unless `readOnly` is set to false, the database will be opened in
 * read-write mode and fall back to read-only if that is not possible.
 *
 * @param filePath path to the file
 * @param key composite key for unlocking the database
 * @param readOnly open in read-only mode
 * @param error error message in case of failure
 * @return true on success
 */
bool Database::open(const QString& filePath, QSharedPointer<const CompositeKey> key, QString* error, bool readOnly)
{
    if (isInitialized() && m_modified) {
        emit databaseDiscarded();
    }

    setEmitModified(false);

    QFile dbFile(filePath);
    if (!dbFile.exists()) {
        if (error) {
            *error = tr("File %1 does not exist.").arg(filePath);
        }
        return false;
    }

    if (!readOnly && !dbFile.open(QIODevice::ReadWrite)) {
        readOnly = true;
    }

    if (!dbFile.isOpen() && !dbFile.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = tr("Unable to open file %1.").arg(filePath);
        }
        return false;
    }

    KeePass2Reader reader;
    bool ok = reader.readDatabase(&dbFile, std::move(key), this);
    if (reader.hasError()) {
        if (error) {
            *error = tr("Error while reading the database: %1").arg(reader.errorString());
        }
        return false;
    }

    setReadOnly(readOnly);
    setFilePath(filePath);
    dbFile.close();

    setInitialized(ok);
    markAsClean();

    setEmitModified(true);
    return ok;
}

/**
 * Save the database back to the file is has been opened from.
 * This method behaves the same as its overloads.
 *
 * @see Database::save(const QString&, bool, bool, QString*)
 *
 * @param atomic Use atomic file transactions
 * @param backup Backup the existing database file, if exists
 * @param error error message in case of failure
 * @return true on success
 */
bool Database::save(QString* error, bool atomic, bool backup)
{
    Q_ASSERT(!m_data.filePath.isEmpty());
    if (m_data.filePath.isEmpty()) {
        if (error) {
            *error = tr("Could not save, database has no file name.");
        }
        return false;
    }

    return save(m_data.filePath, error, atomic, backup);
}

/**
 * Save the database to a file.
 *
 * This function uses QTemporaryFile instead of QSaveFile due to a bug
 * in Qt (https://bugreports.qt.io/browse/QTBUG-57299) that may prevent
 * the QSaveFile from renaming itself when using Dropbox, Drive, or OneDrive.
 *
 * The risk in using QTemporaryFile is that the rename function is not atomic
 * and may result in loss of data if there is a crash or power loss at the
 * wrong moment.
 *
 * @param filePath Absolute path of the file to save
 * @param atomic Use atomic file transactions
 * @param backup Backup the existing database file, if exists
 * @param error error message in case of failure
 * @return true on success
 */
bool Database::save(const QString& filePath, QString* error, bool atomic, bool backup)
{
    Q_ASSERT(!m_data.isReadOnly);
    if (m_data.isReadOnly) {
        return false;
    }

    if (atomic) {
        QSaveFile saveFile(filePath);
        if (saveFile.open(QIODevice::WriteOnly)) {
            // write the database to the file
            if (!writeDatabase(&saveFile, error)) {
                return false;
            }

            if (backup) {
                backupDatabase(filePath);
            }

            if (saveFile.commit()) {
                // successfully saved database file
                setFilePath(filePath);
                return true;
            }
        }
        if (error) {
            *error = saveFile.errorString();
        }
    } else {
        QTemporaryFile tempFile;
        if (tempFile.open()) {
            // write the database to the file
            if (!writeDatabase(&tempFile, error)) {
                return false;
            }

            tempFile.close(); // flush to disk

            if (backup) {
                backupDatabase(filePath);
            }

            // Delete the original db and move the temp file in place
            QFile::remove(filePath);
#ifdef Q_OS_LINUX
            // workaround to make this workaround work, see: https://bugreports.qt.io/browse/QTBUG-64008
            if (tempFile.copy(filePath)) {
                // successfully saved database file
                return true;
            }
#else
            if (tempFile.rename(filePath)) {
                // successfully saved database file
                tempFile.setAutoRemove(false);
                setFilePath(filePath);
                return true;
            }
#endif
        }
        if (error) {
            *error = tempFile.errorString();
        }
    }

    // Saving failed
    return false;
}

bool Database::writeDatabase(QIODevice* device, QString* error)
{
    Q_ASSERT(!m_data.isReadOnly);
    if (m_data.isReadOnly) {
        if (error) {
            *error = tr("File cannot be written as it is opened in read-only mode.");
        }
        return false;
    }

    KeePass2Writer writer;
    setEmitModified(false);
    writer.writeDatabase(device, this);
    setEmitModified(true);

    if (writer.hasError()) {
        // the writer failed
        if (error) {
            *error = writer.errorString();
        }
        return false;
    }

    markAsClean();
    return true;
}

/**
 * Remove the old backup and replace it with a new one
 * backups are named <filename>.old.kdbx
 *
 * @param filePath Path to the file to backup
 * @return true on success
 */
bool Database::backupDatabase(const QString& filePath)
{
    QString backupFilePath = filePath;
    auto re = QRegularExpression("\\.kdbx$|(?<!\\.kdbx)$", QRegularExpression::CaseInsensitiveOption);
    backupFilePath.replace(re, ".old.kdbx");
    QFile::remove(backupFilePath);
    return QFile::copy(filePath, backupFilePath);
}

bool Database::isReadOnly() const
{
    return m_data.isReadOnly;
}

void Database::setReadOnly(bool readOnly)
{
    m_data.isReadOnly = readOnly;
}

/**
 * Returns true if database has been fully decrypted and populated, i.e. if
 * it's not just an empty default instance.
 *
 * @return true if database has been fully initialized
 */
bool Database::isInitialized() const
{
    return m_initialized;
}

/**
 * @param initialized true to mark database as initialized
 */
void Database::setInitialized(bool initialized)
{
    m_initialized = initialized;
}

Group* Database::rootGroup()
{
    return m_rootGroup;
}

const Group* Database::rootGroup() const
{
    return m_rootGroup;
}

/**
 * Sets group as the root group and takes ownership of it.
 * Warning: Be careful when calling this method as it doesn't
 *          emit any notifications so e.g. models aren't updated.
 *          The caller is responsible for cleaning up the previous
            root group.
 */
void Database::setRootGroup(Group* group)
{
    Q_ASSERT(group);

    if (isInitialized() && m_modified) {
        emit databaseDiscarded();
    }

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

QString Database::filePath() const
{
    return m_data.filePath;
}

void Database::setFilePath(const QString& filePath)
{
    if (filePath == m_data.filePath) {
        return;
    }

    if (s_filePathMap.contains(m_data.filePath)) {
        s_filePathMap.remove(m_data.filePath);
    }
    QString oldPath = m_data.filePath;
    m_data.filePath = filePath;
    s_filePathMap.insert(m_data.filePath, this);

    emit filePathChanged(oldPath, filePath);
}

QList<DeletedObject> Database::deletedObjects()
{
    return m_deletedObjects;
}

const QList<DeletedObject>& Database::deletedObjects() const
{
    return m_deletedObjects;
}

bool Database::containsDeletedObject(const QUuid& uuid) const
{
    for (const DeletedObject& currentObject : m_deletedObjects) {
        if (currentObject.uuid == uuid) {
            return true;
        }
    }
    return false;
}

bool Database::containsDeletedObject(const DeletedObject& object) const
{
    for (const DeletedObject& currentObject : m_deletedObjects) {
        if (currentObject.uuid == object.uuid) {
            return true;
        }
    }
    return false;
}

void Database::setDeletedObjects(const QList<DeletedObject>& delObjs)
{
    if (m_deletedObjects == delObjs) {
        return;
    }
    m_deletedObjects = delObjs;
}

void Database::addDeletedObject(const DeletedObject& delObj)
{
    Q_ASSERT(delObj.deletionTime.timeSpec() == Qt::UTC);
    m_deletedObjects.append(delObj);
}

void Database::addDeletedObject(const QUuid& uuid)
{
    DeletedObject delObj;
    delObj.deletionTime = Clock::currentDateTimeUtc();
    delObj.uuid = uuid;

    addDeletedObject(delObj);
}

const QUuid& Database::cipher() const
{
    return m_data.cipher;
}

Database::CompressionAlgorithm Database::compressionAlgorithm() const
{
    return m_data.compressionAlgorithm;
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
    if (m_data.key) {
        m_data.masterSeed = masterSeed;
        return m_data.key->challenge(masterSeed, m_data.challengeResponseKey);
    }
    return false;
}

void Database::setCipher(const QUuid& cipher)
{
    Q_ASSERT(!cipher.isNull());

    m_data.cipher = cipher;
}

void Database::setCompressionAlgorithm(Database::CompressionAlgorithm algo)
{
    Q_ASSERT(static_cast<quint32>(algo) <= CompressionAlgorithmMax);

    m_data.compressionAlgorithm = algo;
}

/**
 * Set and transform a new encryption key.
 *
 * @param key key to set and transform or nullptr to reset the key
 * @param updateChangedTime true to update database change time
 * @param updateTransformSalt true to update the transform salt
 * @return true on success
 */
bool Database::setKey(const QSharedPointer<const CompositeKey>& key, bool updateChangedTime, bool updateTransformSalt)
{
    Q_ASSERT(!m_data.isReadOnly);

    if (!key) {
        m_data.key.reset();
        m_data.transformedMasterKey = {};
        m_data.hasKey = false;
        return true;
    }

    if (updateTransformSalt) {
        m_data.kdf->randomizeSeed();
        Q_ASSERT(!m_data.kdf->seed().isEmpty());
    }

    QByteArray oldTransformedMasterKey = m_data.transformedMasterKey;
    QByteArray transformedMasterKey;
    if (!key->transform(*m_data.kdf, transformedMasterKey)) {
        return false;
    }

    m_data.key = key;
    m_data.transformedMasterKey = transformedMasterKey;
    m_data.hasKey = true;
    if (updateChangedTime) {
        m_metadata->setMasterKeyChanged(Clock::currentDateTimeUtc());
    }

    if (oldTransformedMasterKey != m_data.transformedMasterKey) {
        markAsModified();
    }

    return true;
}

bool Database::hasKey() const
{
    return m_data.hasKey;
}

bool Database::verifyKey(const QSharedPointer<CompositeKey>& key) const
{
    Q_ASSERT(hasKey());

    if (!m_data.challengeResponseKey.isEmpty()) {
        QByteArray result;

        if (!key->challenge(m_data.masterSeed, result)) {
            // challenge failed, (YubiKey?) removed?
            return false;
        }

        if (m_data.challengeResponseKey != result) {
            // wrong response from challenged device(s)
            return false;
        }
    }

    return (m_data.key->rawKey() == key->rawKey());
}

QVariantMap& Database::publicCustomData()
{
    return m_data.publicCustomData;
}

const QVariantMap& Database::publicCustomData() const
{
    return m_data.publicCustomData;
}

void Database::setPublicCustomData(const QVariantMap& customData)
{
    Q_ASSERT(!m_data.isReadOnly);
    m_data.publicCustomData = customData;
}

void Database::createRecycleBin()
{
    Q_ASSERT(!m_data.isReadOnly);
    Group* recycleBin = Group::createRecycleBin();
    recycleBin->setParent(rootGroup());
    m_metadata->setRecycleBin(recycleBin);
}

void Database::recycleEntry(Entry* entry)
{
    Q_ASSERT(!m_data.isReadOnly);
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
    Q_ASSERT(!m_data.isReadOnly);
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
    Q_ASSERT(!m_data.isReadOnly);
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

void Database::setEmitModified(bool value)
{
    if (m_emitModified && !value) {
        m_timer->stop();
    }

    m_emitModified = value;
}

bool Database::isModified() const
{
    return m_modified;
}

void Database::markAsModified()
{
    if (isReadOnly()) {
        return;
    }

    m_modified = true;
    if (m_emitModified) {
        startModifiedTimer();
    }
}

void Database::markAsClean()
{
    bool emitSignal = m_modified;
    m_modified = false;
    if (emitSignal) {
        emit databaseSaved();
    }
}

/**
 * @param uuid UUID of the database
 * @return pointer to the database or nullptr if no such database exists
 */
Database* Database::databaseByUuid(const QUuid& uuid)
{
    return s_uuidMap.value(uuid, nullptr);
}

/**
 * @param filePath file path of the database
 * @return pointer to the database or nullptr if the database has not been opened
 */
Database* Database::databaseByFilePath(const QString& filePath)
{
    return s_filePathMap.value(filePath, nullptr);
}

void Database::startModifiedTimer()
{
    Q_ASSERT(!m_data.isReadOnly);

    if (!m_emitModified) {
        return;
    }

    if (m_timer->isActive()) {
        m_timer->stop();
    }
    m_timer->start(150);
}

QSharedPointer<const CompositeKey> Database::key() const
{
    return m_data.key;
}

QSharedPointer<Kdf> Database::kdf() const
{
    return m_data.kdf;
}

void Database::setKdf(QSharedPointer<Kdf> kdf)
{
    Q_ASSERT(!m_data.isReadOnly);
    m_data.kdf = std::move(kdf);
}

bool Database::changeKdf(const QSharedPointer<Kdf>& kdf)
{
    Q_ASSERT(!m_data.isReadOnly);

    kdf->randomizeSeed();
    QByteArray transformedMasterKey;
    if (!m_data.key) {
        m_data.key = QSharedPointer<CompositeKey>::create();
    }
    if (!m_data.key->transform(*kdf, transformedMasterKey)) {
        return false;
    }

    setKdf(kdf);
    m_data.transformedMasterKey = transformedMasterKey;
    markAsModified();

    return true;
}
