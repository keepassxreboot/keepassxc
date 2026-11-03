/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "core/AsyncTask.h"
#include "core/FileWatcher.h"
#include "core/Group.h"
#include "crypto/Random.h"
#include "format/KdbxXmlReader.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "streams/HashingStream.h"

#include <QFileInfo>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScopeGuard>
#include <QTemporaryFile>
#include <QTimer>
#include <QVector>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace
{
#ifdef Q_OS_WIN
    /**
     * QFile supports long file names. Convert paths to extended path syntax.
     */
    QString nativeFilePath(const QString& path)
    {
        const auto native = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
        const auto prefix = R"(\\?\)";
        if (native.startsWith(prefix)) {
            return native;
        }
        return prefix + (native.startsWith(R"(\\)") ? R"(UNC\)" + native.mid(2) : native);
    }

    /**
     * Open sibling file with a native handle, which we can keep open to prevent other processes
     * from intermittently acquiring a lock on it.
     *
     * @param path sibling file path
     * @param file QFile object to assign handle to
     * @param error error string
     * @return true on success
     */
    bool openSibling(const QString& path, QFile& file, QString& error)
    {
        const auto nativePath = nativeFilePath(path).toStdWString();
        const auto handle = CreateFileW(nativePath.c_str(),
                                        GENERIC_READ | GENERIC_WRITE | DELETE,
                                        0,
                                        nullptr,
                                        CREATE_NEW,
                                        FILE_ATTRIBUTE_NORMAL,
                                        nullptr);
        if (handle == INVALID_HANDLE_VALUE) {
            const auto code = GetLastError();
            error = Database::tr("Could not create temporary file (error code: %1)").arg(code);
            return false;
        }
        const auto descriptor = _open_osfhandle(reinterpret_cast<intptr_t>(handle), _O_RDWR | _O_BINARY);
        if (descriptor == -1) {
            error = QString::fromLocal8Bit(std::strerror(errno));
            CloseHandle(handle);
            QFile::remove(path);
            return false;
        }
        // The descriptor owns the native handle. QFile takes ownership of both on success.
        if (!file.open(descriptor, QIODevice::WriteOnly, QFileDevice::AutoCloseHandle)) {
            error = file.errorString();
            _close(descriptor);
            QFile::remove(path);
            return false;
        }
        return true;
    }
#endif

    /**
     * Flush file contents and then force a disk sync.
     *
     * @param file File to sync
     * @param error Error string
     * @return true on success
     */
    bool syncFile(QFile& file, QString& error)
    {
        if (!file.flush()) {
            error = file.errorString();
            return false;
        }
        const auto descriptor = file.handle();
        if (descriptor < 0) {
            error = Database::tr("Could not obtain descriptor for syncing file.");
            return false;
        }
#ifdef Q_OS_WIN
        const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(descriptor));
        if (!FlushFileBuffers(handle)) {
            error = Database::tr("Error syncing file (error code: %1)").arg(GetLastError());
            return false;
        }
#else
        int result;
        do {
            result = ::fsync(descriptor);
        } while (result == -1 && errno == EINTR);
        if (result == -1) {
            error = Database::tr("Error syncing file: %1").arg(QString::fromLocal8Bit(std::strerror(errno)));
            return false;
        }
#endif
        return true;
    }

    /**
     * Atomically replace a file using native OS APIs.
     *
     * Unlike `QFile::rename()`, this function allows overwrite semantics without a copy / delete fallback.
     * Atomicity guarantees depend on the target filesystem.
     *
     * @param source open source file (must have DELETE access on Windows)
     * @param sourcePath source file path
     * @param target target to replace
     * @param error error string
     * @return true on success
     */
    bool replaceFile(QFile& source, const QString& sourcePath, const QString& target, QString& error)
    {
#ifdef Q_OS_WIN
        // Replace directly through the exclusive handle. Never close and reopen the sibling.
        // Unlike ReplaceFileW, this does not merge the destination's ACLs or other metadata.
        const auto nativeTarget = nativeFilePath(target);
        const auto nameBytes = nativeTarget.size() * sizeof(ushort);
        const auto bufferSize = sizeof(FILE_RENAME_INFO) + nameBytes;
        // Use suitably aligned storage for the variable-length FILE_RENAME_INFO structure.
        QVector<quint64> buffer((bufferSize + sizeof(quint64) - 1) / sizeof(quint64), 0);
        auto* renameInfo = reinterpret_cast<FILE_RENAME_INFO*>(buffer.data());
        renameInfo->ReplaceIfExists = TRUE;
        renameInfo->FileNameLength = static_cast<DWORD>(nameBytes);
        std::memcpy(renameInfo->FileName, nativeTarget.utf16(), nameBytes);
        const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(source.handle()));
        if (!SetFileInformationByHandle(handle, FileRenameInfo, renameInfo, static_cast<DWORD>(bufferSize))) {
            const auto code = GetLastError();
            error = Database::tr("Could not replace file (error code: %1)").arg(code);
            return false;
        }
        Q_UNUSED(sourcePath);
#else
        Q_UNUSED(source);
        if (::rename(QFile::encodeName(sourcePath).constData(), QFile::encodeName(target).constData()) != 0) {
            error = Database::tr("Could not replace file: %1").arg(QString::fromLocal8Bit(std::strerror(errno)));
            return false;
        }
#endif
        return true;
    }
} // namespace

QHash<QUuid, QPointer<Database>> Database::s_uuidMap;

Database::Database()
    : m_metadata(new Metadata(this))
    , m_data()
    , m_rootGroup(nullptr)
    , m_fileWatcher(new FileWatcher(this))
    , m_uuid(QUuid::createUuid())
{
    // setup modified timer
    m_modifiedTimer.setSingleShot(true);
    connect(this, &Database::emitModifiedChanged, this, [this](bool value) {
        if (!value) {
            stopModifiedTimer();
        }
    });
    connect(&m_modifiedTimer, &QTimer::timeout, this, &Database::emitModified);

    // other signals
    connect(m_metadata, &Metadata::modified, this, &Database::markAsModified);
    connect(this, &Database::databaseOpened, this, [this]() {
        updateCommonUsernames();
        updateTagList();
        updateCustomAttributeKeys();
    });
    connect(this, &Database::modified, this, [this] { updateTagList(); });
    connect(this, &Database::databaseSaved, this, [this] {
        updateCommonUsernames();
        updateCustomAttributeKeys();
    });
    connect(m_fileWatcher, &FileWatcher::fileChanged, this, [this] { emit databaseFileChanged(false); });

    // static uuid map
    s_uuidMap.insert(m_uuid, this);

    // block modified signal and set root group
    setEmitModified(false);

    // Note: oldGroup is nullptr but need to respect return value capture
    auto oldGroup = setRootGroup(new Group());
    Q_UNUSED(oldGroup)

    m_modified = false;
    setEmitModified(true);
}

Database::Database(const QString& filePath)
    : Database()
{
    setFilePath(filePath);
}

Database::~Database()
{
    releaseData();
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
 * @param error error message in case of failure
 * @return true on success
 */
bool Database::open(QSharedPointer<const CompositeKey> key, QString* error)
{
    return open(m_data.filePath, std::move(key), error);
}

/**
 * Open the database from a file.
 * Unless `readOnly` is set to false, the database will be opened in
 * read-write mode and fall back to read-only if that is not possible.
 *
 * If key is provided as null, only headers will be read.
 *
 * @param filePath path to the file
 * @param key composite key for unlocking the database
 * @param error error message in case of failure
 * @return true on success
 */
bool Database::open(const QString& filePath, QSharedPointer<const CompositeKey> key, QString* error)
{
    if (filePath.isEmpty()) {
        if (error) {
            *error = tr("No file path was provided.");
        }
        return false;
    }

    QFile dbFile(filePath);
    if (!dbFile.exists()) {
        if (error) {
            *error = tr("File %1 does not exist.").arg(filePath);
        }
        return false;
    }

    // Don't autodetect read-only mode, as it triggers an upstream bug.
    // See https://github.com/keepassxreboot/keepassxc/issues/803
    // if (!readOnly && !dbFile.open(QIODevice::ReadWrite)) {
    //     readOnly = true;
    // }
    //
    // if (!dbFile.isOpen() && !dbFile.open(QIODevice::ReadOnly)) {
    if (!dbFile.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = tr("Unable to open file %1.").arg(filePath);
        }
        return false;
    }

    setEmitModified(false);

    // update the hash of the first block
    m_fileBlockHash.clear();
    auto fileBlockData = dbFile.peek(kFileBlockToHashSizeBytes);
    if (fileBlockData.size() != kFileBlockToHashSizeBytes) {
        if (dbFile.size() >= kFileBlockToHashSizeBytes) {
            if (error) {
                *error = tr("Database file read error.");
            }
            return false;
        }
    } else {
        m_fileBlockHash = QCryptographicHash::hash(fileBlockData, QCryptographicHash::Md5);
    }

    KeePass2Reader reader;
    if (!reader.readDatabase(&dbFile, std::move(key), this)) {
        if (error) {
            *error = tr("Error while reading the database: %1").arg(reader.errorString());
        }
        return false;
    }

    setFilePath(filePath);
    dbFile.close();

    markAsClean();

    emit databaseOpened();
    m_fileWatcher->start(canonicalFilePath(), 30, 1);
    setEmitModified(true);

    return true;
}

/**
 * KDBX format version.
 */
quint32 Database::formatVersion() const
{
    return m_data.formatVersion;
}

void Database::setFormatVersion(quint32 version)
{
    m_data.formatVersion = version;
}

/**
 * Whether the KDBX minor version is greater than the newest supported.
 */
bool Database::hasMinorVersionMismatch() const
{
    return m_data.formatVersion > KeePass2::FILE_VERSION_MAX;
}

bool Database::isSaving()
{
    bool locked = m_saveMutex.tryLock();
    if (locked) {
        m_saveMutex.unlock();
    }
    return !locked;
}

/**
 * Save the database to the current file path. It is an error to call this function
 * if no file path has been defined.
 *
 * @param error Error message in case of failure
 * @param savingMethod Saving method
 * @param backupFilePath Absolute file path to write the backup file to. Pass an empty QString to disable backup.
 * @return true on success
 */
bool Database::save(SaveAction savingMethod, const QString& backupFilePath, QString* error)
{
    Q_ASSERT(!m_data.filePath.isEmpty());
    if (m_data.filePath.isEmpty()) {
        if (error) {
            *error = tr("Could not save, database does not point to a valid file.");
        }
        return false;
    }

    return saveAs(m_data.filePath, savingMethod, backupFilePath, error);
}

/**
 * Save the database to a specific file using the specified saving method.
 *
 * Atomic saves may fail due to a bug in Qt (https://bugreports.qt.io/browse/QTBUG-57299).
 * In that case, `TempFile` may be used as an alternative save method. It will instead write
 * to a (local) temporary file, stream the serialised contents to a named sibling of the destination,
 * sync, and then replace the destination with an atomic rename. Using a temporary file
 * has the advantage of keeping a temporary backup copy on the local file system in case of a failure.
 * Streaming the byte contents into the destination avoids another Qt bug with `QFile::copy()` /
 * `QFile::rename()`, which can trigger on certain remote file systems that don't support hard-links.
 * TempFile also tries to keep the sibling handle open to prevent intermittent locking by other processes.
 *
 * Failures to save are reported to the caller, which can decide to offer the user an
 * alternative saving method.
 *
 * Direct write is the most dangerous alternative save method, which will truncate and
 * directly write to the destination.
 *
 * @param filePath Absolute path of the file to save
 * @param error Error message in case of failure
 * @param savingMethod Selected saving method
 * @param backupFilePath Absolute path to the location where the backup should be stored. Passing
 *                       an empty string disables backup.
 * @return true on success
 */
bool Database::saveAs(const QString& filePath, SaveAction savingMethod, const QString& backupFilePath, QString* error)
{
    // Disallow overlapping save operations
    if (isSaving()) {
        if (error) {
            *error = tr("Database save is already in progress.");
        }
        return false;
    }

    // Never save an uninitialized database
    if (!isInitialized()) {
        if (error) {
            *error = tr("Could not save, database has not been initialized!");
        }
        return false;
    }

    // Make sure we don't overwrite external modifications unless explicitly allowed
    if (!m_ignoreFileChangesUntilSaved && !m_fileBlockHash.isEmpty() && filePath == m_data.filePath) {
        QFile dbFile(filePath);
        if (dbFile.exists()) {
            if (!dbFile.open(QIODevice::ReadOnly)) {
                if (error) {
                    *error = tr("Unable to open file %1.").arg(filePath);
                }
                return false;
            }
            auto fileBlockData = dbFile.read(kFileBlockToHashSizeBytes);
            if (fileBlockData.size() == kFileBlockToHashSizeBytes) {
                auto hash = QCryptographicHash::hash(fileBlockData, QCryptographicHash::Md5);
                if (m_fileBlockHash != hash) {
                    if (error) {
                        *error = tr("Database file has unmerged changes.");
                    }
                    // emit the databaseFileChanged(true) signal async
                    QMetaObject::invokeMethod(this, "databaseFileChanged", Qt::QueuedConnection, Q_ARG(bool, true));
                    return false;
                }
            } else if (dbFile.size() >= kFileBlockToHashSizeBytes) {
                if (error) {
                    *error = tr("Database file read error.");
                }
                return false;
            }
        }
    }

    // Clear read-only flag
    m_fileWatcher->stop();

    // Add random data to prevent side-channel data deduplication attacks
    int length = Random::instance()->randomUIntRange(64, 512);
    m_metadata->customData()->set(CustomData::RandomSlug, Random::instance()->randomArray(length).toHex());

    // Prevent destructive operations while saving
    QMutexLocker locker(&m_saveMutex);

    QFileInfo fileInfo(filePath);
    auto realFilePath = fileInfo.exists() ? fileInfo.canonicalFilePath() : fileInfo.absoluteFilePath();
    bool isNewFile = !QFile::exists(realFilePath);

#ifdef Q_OS_WIN
    bool isHidden = fileInfo.isHidden();
#endif

    bool ok =
        AsyncTask::runAndWaitForFuture([&] { return performSave(realFilePath, savingMethod, backupFilePath, error); });
    if (ok) {
        setFilePath(filePath);
        markAsClean();
        if (isNewFile) {
            QFile::setPermissions(realFilePath, QFile::ReadUser | QFile::WriteUser);
        }

#ifdef Q_OS_WIN
        if (isHidden) {
            SetFileAttributes(realFilePath.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
        }
#endif
        m_ignoreFileChangesUntilSaved = false;
        m_fileWatcher->start(realFilePath, 30, 1);
    } else {
        // Saving failed, don't rewatch file since it does not represent our database
        markAsModified();
    }

    return ok;
}

bool Database::performSave(const QString& filePath,
                           SaveAction savingMethod,
                           const QString& backupFilePath,
                           QString* error)
{
    if (!backupFilePath.isNull()) {
        backupDatabase(filePath, backupFilePath);
    }

    QFileInfo info(filePath);
    auto createTime = info.exists() ? info.birthTime() : QDateTime::currentDateTime();

    switch (savingMethod) {
    case Atomic: {
        QSaveFile saveFile(filePath);
        if (saveFile.open(QIODevice::WriteOnly)) {
            HashingStream hashingStream(&saveFile, QCryptographicHash::Md5, kFileBlockToHashSizeBytes);
            if (!hashingStream.open(QIODevice::WriteOnly)) {
                return false;
            }
            // write the database to the file
            if (!writeDatabase(&hashingStream, error)) {
                return false;
            }

            // Retain original creation time
            saveFile.setFileTime(createTime, QFile::FileBirthTime);

            if (saveFile.commit()) {
                // store the new hash
                m_fileBlockHash = hashingStream.hashingResult();

                // successfully saved database file
                return true;
            }
        }

        if (error) {
            *error = saveFile.errorString();
        }
        break;
    }
    case TempFile: {
        // Since replacing the destination file is a directory-level operation on POSIX systems,
        // we may inadvertently overwrite read-only files if the parent directory permissions allow it.
        // Check file permissions first and error out if file is marked as read-only.
        if (info.isFile() && !info.isWritable()) {
            if (error) {
                *error = tr("Database file is read-only.");
            }
            return false;
        }

        QTemporaryFile tempFile;
        if (!tempFile.open()) {
            if (error) {
                *error = tempFile.errorString();
            }
            return false;
        }

        // Materialize the temporary file before writing so it has a usable recovery
        // path even if writing or syncing subsequently fails.
        const auto recoveryPath = tempFile.fileName();
        if (recoveryPath.isEmpty() || !QFile::exists(recoveryPath)) {
            if (error) {
                *error = tr("Could not create a named temporary file: %1").arg(tempFile.errorString());
            }
            return false;
        }

        HashingStream hashingStream(&tempFile, QCryptographicHash::Md5, kFileBlockToHashSizeBytes);
        if (!hashingStream.open(QIODevice::WriteOnly)) {
            if (error) {
                *error = hashingStream.errorString();
            }
            return false;
        }
        if (!writeDatabase(&hashingStream, error)) {
            return false;
        }

        // Fail with error message and return path to retained backup copy.
        const auto fail = [&](const QString& message) {
            tempFile.setAutoRemove(false);
            if (error) {
                *error = tr("%1\nBackup database located at %2").arg(message, recoveryPath);
            }
            return false;
        };

        QString fileError;
        if (!syncFile(tempFile, fileError)) {
            return fail(fileError);
        }

        // Stream contents from temporary file to sibling. Don't use QFile::copy() or QFile::rename(), since they
        // are not atomic and can fail on destination file systems that don't support hard-linking or O_TMPFILE renames.
        const auto siblingName =
            QStringLiteral(".~%1.tmp.%2").arg(info.fileName()).arg(Random::instance()->randomArray(8).toHex());
        const auto siblingPath = info.absoluteDir().filePath(siblingName);
        QFile sibling(siblingPath);
#ifdef Q_OS_WIN
        if (!openSibling(siblingPath, sibling, fileError)) {
            return fail(fileError);
        }
#else
        if (!sibling.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
            return fail(sibling.errorString());
        }
#endif
        auto cleanup = qScopeGuard([&] {
            sibling.close();
            QFile::remove(siblingPath);
        });
        // A QFile opened from a descriptor has no path in its
        // file engine on Windows. So, use another (unopened) handle to the sibling to set permissions.
        QFile siblingPermissions(siblingPath);
        if (!siblingPermissions.setPermissions(QFile::ReadUser | QFile::WriteUser)) {
            return fail(siblingPermissions.errorString());
        }
        // Stream temp file contents
        if (!tempFile.reset()) {
            return fail(tempFile.errorString());
        }
        while (!tempFile.atEnd()) {
            const auto chunk = tempFile.read(64 << 10);
            if (tempFile.error() != QFile::NoError) {
                return fail(tempFile.errorString());
            }
            if (sibling.write(chunk) != chunk.size()) {
                return fail(sibling.errorString());
            }
        }
        // Keep original permissions.
        if (info.exists() && !siblingPermissions.setPermissions(info.permissions())) {
            return fail(siblingPermissions.errorString());
        }
        // Best effort. Qt does not support setting FileBirthTime on Unix.
        if (!sibling.setFileTime(createTime, QFile::FileBirthTime)) {
            sibling.unsetError();
        }
        if (!syncFile(sibling, fileError)) {
            return fail(fileError);
        }
        if (sibling.error() != QFile::NoError) {
            return fail(sibling.errorString());
        }
        if (!replaceFile(sibling, siblingPath, filePath, fileError)) {
            return fail(fileError);
        }
        sibling.close();
        cleanup.dismiss();

        m_fileBlockHash = hashingStream.hashingResult();
        if (error) {
            error->clear();
        }
        return true;
    }
    case DirectWrite: {
        QBuffer dbBuffer;
        dbBuffer.open(QIODevice::WriteOnly);
        HashingStream hashingStream(&dbBuffer, QCryptographicHash::Md5, kFileBlockToHashSizeBytes);
        if (!hashingStream.open(QIODevice::WriteOnly)) {
            if (error) {
                *error = hashingStream.errorString();
            }
            return false;
        }
        if (!writeDatabase(&hashingStream, error)) {
            return false;
        }

        // Open the original database file for direct-write
        QFile dbFile(filePath);
        if (dbFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            dbFile.write(dbBuffer.data());
            dbFile.close();
            // store the new hash
            m_fileBlockHash = hashingStream.hashingResult();
            return true;
        }
        if (error) {
            *error = dbFile.errorString();
        }
        break;
    }
    }

    // Saving failed
    return false;
}

bool Database::writeDatabase(QIODevice* device, QString* error)
{
    Q_ASSERT(m_data.key);
    Q_ASSERT(m_data.transformedDatabaseKey);

    PasswordKey oldTransformedKey;
    if (m_data.key->isEmpty()) {
        oldTransformedKey.setRawKey(m_data.transformedDatabaseKey->rawKey());
    }

    KeePass2Writer writer;
    setEmitModified(false);
    writer.writeDatabase(device, this);
    setEmitModified(true);

    if (writer.hasError()) {
        if (error) {
            *error = writer.errorString();
        }
        return false;
    }

    QByteArray newKey = m_data.transformedDatabaseKey->rawKey();
    Q_ASSERT(!newKey.isEmpty());
    Q_ASSERT(newKey != oldTransformedKey.rawKey());
    if (newKey.isEmpty() || newKey == oldTransformedKey.rawKey()) {
        if (error) {
            *error = tr("Key not transformed. This is a bug, please report it to the developers.");
        }
        return false;
    }

    return true;
}

bool Database::extract(QByteArray& xmlOutput, QString* error)
{
    KeePass2Writer writer;
    writer.extractDatabase(this, xmlOutput);
    if (writer.hasError()) {
        if (error) {
            *error = writer.errorString();
        }
        return false;
    }

    return true;
}

bool Database::import(const QString& xmlExportPath, QString* error)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_4);
    QFile file(xmlExportPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    reader.readDatabase(&file, this);

    if (reader.hasError()) {
        if (error) {
            *error = reader.errorString();
        }
        return false;
    }

    return true;
}

/**
 * Release all stored group, entry, and meta data of this database.
 *
 * Call this method to ensure all data is cleared even if valid
 * pointers to this Database object are still being held.
 *
 * A previously reparented root group will not be freed.
 */
void Database::releaseData()
{
    // Prevent data release while saving
    Q_ASSERT(!isSaving());
    QMutexLocker locker(&m_saveMutex);

    if (m_modified) {
        emit databaseDiscarded();
    }

    setEmitModified(false);
    m_modified = false;

    s_uuidMap.remove(m_uuid);
    m_uuid = QUuid();

    m_data.clear();
    m_metadata->clear();

    // Reset and delete the root group
    auto oldGroup = setRootGroup(new Group());
    delete oldGroup;

    m_fileWatcher->stop();

    m_deletedObjects.clear();
    m_commonUsernames.clear();
    m_tagList.clear();
    m_customAttributeKeys.clear();

    m_fileBlockHash.clear();
    m_ignoreFileChangesUntilSaved = false;
}

/**
 * Remove the old backup and replace it with a new one. Backup name is taken from destinationFilePath.
 * Non-existing parent directories will be created automatically.
 *
 * @param filePath Path to the file to backup
 * @param destinationFilePath Path to the backup destination file
 * @return true on success
 */
bool Database::backupDatabase(const QString& filePath, const QString& destinationFilePath)
{
    // Ensure that the path to write to actually exists
    auto parentDirectory = QFileInfo(destinationFilePath).absoluteDir();
    if (!parentDirectory.exists()) {
        if (!QDir().mkpath(parentDirectory.absolutePath())) {
            return false;
        }
    }
    auto perms = QFile::permissions(filePath);
    QFile::remove(destinationFilePath);
    bool res = QFile::copy(filePath, destinationFilePath);
    QFile::setPermissions(destinationFilePath, perms);
    return res;
}

/**
 * Returns true if the database key exists, has subkeys, and the
 * root group exists
 *
 * @return true if database has been fully initialized
 */
bool Database::isInitialized() const
{
    return m_data.key && !m_data.key->isEmpty() && m_rootGroup;
}

Group* Database::rootGroup()
{
    return m_rootGroup;
}

const Group* Database::rootGroup() const
{
    return m_rootGroup;
}

/* Set the root group of the database and return
 * the old root group. It is the responsibility
 * of the calling function to dispose of the old
 * root group.
 */
Group* Database::setRootGroup(Group* group)
{
    Q_ASSERT(group);

    if (isInitialized() && isModified()) {
        emit databaseDiscarded();
    }

    auto oldRoot = m_rootGroup;
    m_rootGroup = group;
    m_rootGroup->setParent(this);

    // Initialize the root group if not done already
    if (m_rootGroup->uuid().isNull()) {
        m_rootGroup->setUuid(QUuid::createUuid());
        m_rootGroup->setName(tr("Passwords", "Root group name"));
    }

    return oldRoot;
}

Metadata* Database::metadata()
{
    return m_metadata;
}

const Metadata* Database::metadata() const
{
    return m_metadata;
}

/**
 * Returns the original file path that was provided for
 * this database. This path may not exist, may contain
 * unresolved symlinks, or have malformed slashes.
 *
 * @return original file path
 */
QString Database::filePath() const
{
    return m_data.filePath;
}

/**
 * Returns the canonical file path of this databases'
 * set file path. This returns an empty string if the
 * file does not exist or cannot be resolved.
 *
 * @return canonical file path
 */
QString Database::canonicalFilePath() const
{
    QFileInfo fileInfo(m_data.filePath);
    return fileInfo.canonicalFilePath();
}

void Database::setFilePath(const QString& filePath)
{
    if (filePath != m_data.filePath) {
        QString oldPath = m_data.filePath;
        m_data.filePath = filePath;
        // Don't watch for changes until the next open or save operation
        m_fileWatcher->stop();
        m_ignoreFileChangesUntilSaved = false;
        emit filePathChanged(oldPath, filePath);
    }
}

const QByteArray& Database::fileBlockHash() const
{
    return m_fileBlockHash;
}

void Database::setIgnoreFileChangesUntilSaved(bool ignore)
{
    if (m_ignoreFileChangesUntilSaved != ignore) {
        m_ignoreFileChangesUntilSaved = ignore;
        if (ignore) {
            m_fileWatcher->pause();
        } else {
            m_fileWatcher->resume();
        }
    }
}

bool Database::ignoreFileChangesUntilSaved() const
{
    return m_ignoreFileChangesUntilSaved;
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
    return std::any_of(m_deletedObjects.cbegin(),
                       m_deletedObjects.cend(),
                       [&uuid](const DeletedObject& object) -> bool { return object.uuid == uuid; });
}

bool Database::containsDeletedObject(const DeletedObject& object) const
{
    return containsDeletedObject(object.uuid);
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

const QStringList& Database::commonUsernames() const
{
    return m_commonUsernames;
}

const QStringList& Database::customAttributeKeys() const
{
    return m_customAttributeKeys;
}

const QStringList& Database::tagList() const
{
    return m_tagList;
}

void Database::updateCommonUsernames(int topN)
{
    m_commonUsernames.clear();
    m_commonUsernames.append(rootGroup()->usernamesRecursive(topN));
}

void Database::updateCustomAttributeKeys()
{
    m_customAttributeKeys.clear();
    if (!m_rootGroup) {
        return;
    }

    // Search groups recursively looking for custom attribute keys
    // Use a set to prevent adding duplicates
    QSet<QString> customKeysSet;
    for (auto entry : m_rootGroup->entriesRecursive()) {
        if (!entry->isRecycled()) {
            for (auto key : entry->attributes()->customKeys()) {
                customKeysSet.insert(key);
            }
        }
    }

    m_customAttributeKeys = customKeysSet.values();

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(m_customAttributeKeys.begin(), m_customAttributeKeys.end(), collator);
}

void Database::updateTagList()
{
    m_tagList.clear();
    if (!m_rootGroup) {
        emit tagListUpdated();
        return;
    }

    // Search groups recursively looking for tags
    // Use a set to prevent adding duplicates
    QSet<QString> tagSet;
    for (auto entry : m_rootGroup->entriesRecursive()) {
        if (!entry->isRecycled()) {
            for (auto tag : entry->tagList()) {
                tagSet.insert(tag);
            }
        }
    }

    m_tagList = tagSet.values();

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(m_tagList.begin(), m_tagList.end(), collator);

    emit tagListUpdated();
}

void Database::removeTag(const QString& tag)
{
    if (!m_rootGroup) {
        return;
    }

    for (auto entry : m_rootGroup->entriesRecursive()) {
        entry->removeTag(tag);
    }
}

const QUuid& Database::cipher() const
{
    return m_data.cipher;
}

Database::CompressionAlgorithm Database::compressionAlgorithm() const
{
    return m_data.compressionAlgorithm;
}

QByteArray Database::transformedDatabaseKey() const
{
    Q_ASSERT(m_data.transformedDatabaseKey);
    if (!m_data.transformedDatabaseKey) {
        return {};
    }
    return m_data.transformedDatabaseKey->rawKey();
}

QByteArray Database::challengeResponseKey() const
{
    Q_ASSERT(m_data.challengeResponseKey);
    if (!m_data.challengeResponseKey) {
        return {};
    }
    return m_data.challengeResponseKey->rawKey();
}

bool Database::challengeMasterSeed(const QByteArray& masterSeed)
{
    Q_ASSERT(m_data.key);
    Q_ASSERT(m_data.masterSeed);

    m_keyError.clear();
    if (m_data.key && m_data.masterSeed) {
        m_data.masterSeed->setRawKey(masterSeed);
        QByteArray response;
        bool ok = m_data.key->challenge(masterSeed, response, &m_keyError);
        if (ok && !response.isEmpty()) {
            m_data.challengeResponseKey->setRawKey(response);
        } else if (ok && response.isEmpty()) {
            // no CR key present, make sure buffer is empty
            m_data.challengeResponseKey.reset(new PasswordKey);
        }
        return ok;
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
 * @param transformKey trigger the KDF after setting the key
 * @return true on success
 */
bool Database::setKey(const QSharedPointer<const CompositeKey>& key,
                      bool updateChangedTime,
                      bool updateTransformSalt,
                      bool transformKey)
{
    m_keyError.clear();

    if (!key) {
        m_data.resetKeys();
        return true;
    }

    if (updateTransformSalt) {
        m_data.kdf->randomizeSeed();
        Q_ASSERT(!m_data.kdf->seed().isEmpty());
    }

    PasswordKey oldTransformedDatabaseKey;
    if (m_data.key && !m_data.key->isEmpty()) {
        oldTransformedDatabaseKey.setRawKey(m_data.transformedDatabaseKey->rawKey());
    }

    QByteArray transformedDatabaseKey;

    if (!transformKey) {
        transformedDatabaseKey = QByteArray(oldTransformedDatabaseKey.rawKey());
    } else if (!key->transform(*m_data.kdf, transformedDatabaseKey, &m_keyError)) {
        return false;
    }

    m_data.key = key;
    if (!transformedDatabaseKey.isEmpty()) {
        m_data.transformedDatabaseKey->setRawKey(transformedDatabaseKey);
    }
    if (updateChangedTime) {
        m_metadata->setDatabaseKeyChanged(Clock::currentDateTimeUtc());
    }

    if (oldTransformedDatabaseKey.rawKey() != m_data.transformedDatabaseKey->rawKey()) {
        markAsModified();
    }

    return true;
}

QString Database::keyError()
{
    return m_keyError;
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
    m_data.publicCustomData = customData;
}

void Database::createRecycleBin()
{
    auto recycleBin = new Group();
    recycleBin->setUuid(QUuid::createUuid());
    recycleBin->setParent(rootGroup());
    recycleBin->setName(tr("Recycle Bin"));
    recycleBin->setIcon(Group::RecycleBinIconNumber);
    recycleBin->setSearchingEnabled(Group::Disable);
    recycleBin->setAutoTypeEnabled(Group::Disable);

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

bool Database::isModified() const
{
    return m_modified;
}

bool Database::hasNonDataChanges() const
{
    return m_hasNonDataChange;
}

void Database::markAsModified()
{
    m_modified = true;
    if (modifiedSignalEnabled() && !m_modifiedTimer.isActive()) {
        // Small time delay prevents numerous consecutive saves due to repeated signals
        startModifiedTimer();
    }
}

void Database::markAsClean()
{
    bool emitSignal = m_modified;
    m_modified = false;
    stopModifiedTimer();
    m_hasNonDataChange = false;
    if (emitSignal) {
        emit databaseSaved();
    }
}

void Database::markNonDataChange()
{
    m_hasNonDataChange = true;
    emit databaseNonDataChanged();
}

/**
 * @param uuid UUID of the database
 * @return pointer to the database or nullptr if no such database exists
 */
Database* Database::databaseByUuid(const QUuid& uuid)
{
    return s_uuidMap.value(uuid, nullptr);
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
    m_data.kdf = std::move(kdf);
    setFormatVersion(KeePass2Writer::kdbxVersionRequired(this, true, m_data.kdf.isNull()));
}

bool Database::changeKdf(const QSharedPointer<Kdf>& kdf)
{
    kdf->randomizeSeed();
    QByteArray transformedDatabaseKey;
    if (!m_data.key) {
        m_data.key = QSharedPointer<CompositeKey>::create();
    }
    if (!m_data.key->transform(*kdf, transformedDatabaseKey)) {
        return false;
    }

    setKdf(kdf);
    m_data.transformedDatabaseKey->setRawKey(transformedDatabaseKey);
    markAsModified();

    return true;
}

// Prevent warning about QTimer not allowed to be started/stopped from other thread
void Database::startModifiedTimer()
{
    QMetaObject::invokeMethod(&m_modifiedTimer, "start", Q_ARG(int, 150));
}

// Prevent warning about QTimer not allowed to be started/stopped from other thread
void Database::stopModifiedTimer()
{
    QMetaObject::invokeMethod(&m_modifiedTimer, "stop");
}

QUuid Database::publicUuid()
{
    // This feature requires KDBX4
    if (m_data.formatVersion < KeePass2::FILE_VERSION_4) {
        // Return the file path hash as a UUID for KDBX3
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        hasher.addData(filePath().toUtf8());
        return QUuid::fromRfc4122(hasher.result().left(16));
    }

    if (!publicCustomData().contains("KPXC_PUBLIC_UUID")) {
        publicCustomData().insert("KPXC_PUBLIC_UUID", QUuid::createUuid().toRfc4122());
        markAsModified();
    }

    return QUuid::fromRfc4122(publicCustomData()["KPXC_PUBLIC_UUID"].toByteArray());
}

QString Database::publicName()
{
    return publicCustomData().value("KPXC_PUBLIC_NAME").toString();
}

void Database::setPublicName(const QString& name)
{
    if (name.isEmpty()) {
        publicCustomData().remove("KPXC_PUBLIC_NAME");
    } else {
        publicCustomData().insert("KPXC_PUBLIC_NAME", name);
    }
    markAsModified();
}

QString Database::publicColor()
{
    return publicCustomData().value("KPXC_PUBLIC_COLOR").toString();
}

void Database::setPublicColor(const QString& color)
{
    if (color.isEmpty()) {
        publicCustomData().remove("KPXC_PUBLIC_COLOR");
    } else {
        publicCustomData().insert("KPXC_PUBLIC_COLOR", color);
    }
    markAsModified();
}

int Database::publicIcon()
{
    if (publicCustomData().contains("KPXC_PUBLIC_ICON")) {
        return publicCustomData().value("KPXC_PUBLIC_ICON").toInt();
    }
    return -1;
}

void Database::setPublicIcon(int iconIndex)
{
    if (iconIndex < 0) {
        publicCustomData().remove("KPXC_PUBLIC_ICON");
    } else {
        publicCustomData().insert("KPXC_PUBLIC_ICON", iconIndex);
    }
    markAsModified();
}

void Database::markAsTemporaryDatabase()
{
    m_isTemporaryDatabase = true;
}

bool Database::isTemporaryDatabase()
{
    return m_isTemporaryDatabase;
}
