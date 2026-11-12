/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_OFFLINECACHE_H
#define KEEPASSXC_OFFLINECACHE_H

#include <QDateTime>
#include <QList>
#include <QString>

class Database;

/**
 * Local, encrypted mirror of a database file that lives on a cloud or network drive ("cloud mode").
 *
 * Every database gets its own folder below the base directory:
 *
 *   <baseDir>/<hash of original path>/cache.kdbx   latest copy of the database (still encrypted)
 *   <baseDir>/<hash of original path>/state.json   dirty flag, original path, backup version list
 *   <baseDir>/<hash of original path>/versions/    backup versions (at most maxVersions)
 *
 * This class only deals with files and bookkeeping. Opening, saving and merging databases is done by the callers.
 */
class OfflineCache
{
public:
    enum class Override
    {
        Unset,
        Enabled,
        Disabled
    };

    /**
     * Content fingerprints of a database. Both are independent of the random data written on every save, of file
     * timestamps and of volatile fields (last access time, usage count).
     */
    struct Fingerprints
    {
        // Hash over everything in the database (entries, passwords, groups, metadata, ...)
        QString content;
        // Hash over the set of entry and group UUIDs and the database key change time
        // (entries / groups added or deleted, master key changed)
        QString structure;
    };

    struct Version
    {
        QString fileName;
        QDateTime timestamp;
        // Start of the coalescing window; only changes to the content are coalesced
        QDateTime windowStart;
        Fingerprints fingerprints;
        // True if this version was opened because of a structural change (or an explicit snapshot)
        bool structural = false;
    };

    static constexpr int DefaultMaxVersions = 10;
    static constexpr int DefaultCoalesceMinutes = 10;

    explicit OfflineCache(const QString& originalFilePath, const QString& baseDir = configuredBaseDir());

    static OfflineCache forDatabase(const QString& originalFilePath);
    static QString defaultBaseDir();
    static QString configuredBaseDir();
    static int configuredMaxVersions();
    static QString normalizeFilePath(const QString& filePath);

    static Override localOverride(const QString& filePath);
    static void setLocalOverride(const QString& filePath, Override override);
    static bool isCloudDatabase(const Database* db);
    static bool isCloudFilePath(const QString& filePath);
    static bool canFallbackToCache(const QString& filePath);
    static bool isOriginalFileReachable(const QString& filePath, int timeoutMs = 2000);

    static Fingerprints fingerprint(const Database* db);

    QString originalFilePath() const;
    QString directory() const;
    QString cacheFilePath() const;
    QString versionsDirectory() const;

    bool hasCacheFile() const;
    QDateTime cacheLastModified() const;

    int maxVersions() const;
    void setMaxVersions(int maxVersions);

    /**
     * The cache holds changes that have not been written to the original file yet.
     */
    bool isDirty() const;
    bool setDirty(bool dirty);

    QDateTime lastSyncTime() const;
    bool setLastSyncTime(const QDateTime& time = QDateTime::currentDateTime());

    bool isSuggestedByDatabase() const;
    bool setSuggestedByDatabase(bool suggested);

    /**
     * Atomically replace the cache file with a copy of sourceFile.
     */
    bool storeCopyOf(const QString& sourceFile, QString* error = nullptr);

    /**
     * Store sourceFile as a backup version if the content differs from the newest version.
     *
     * - A change of the content within the coalescing window overwrites the newest version.
     * - A structural change (entries / groups added or removed, key changed) or an explicit snapshot always
     *   starts a new version.
     * - Versions beyond maxVersions are deleted, oldest first.
     *
     * @param forceNewVersion always start a new version (use before merges and before replacing the cache)
     * @return true if nothing had to be done or the version was stored
     */
    bool addVersion(const QString& sourceFile,
                    const Fingerprints& fingerprints,
                    bool forceNewVersion = false,
                    const QDateTime& now = QDateTime::currentDateTime(),
                    QString* error = nullptr);

    QList<Version> versions() const;
    bool removeAll();

private:
    struct State
    {
        bool dirty = false;
        bool suggestedByDatabase = false;
        QDateTime lastSyncTime;
        QList<Version> versions;
    };

    State loadState() const;
    bool saveState(const State& state) const;
    QString stateFilePath() const;
    static bool copyAtomically(const QString& source, const QString& destination, QString* error);

    QString m_originalFilePath;
    QString m_directory;
    int m_maxVersions = DefaultMaxVersions;
};

#endif // KEEPASSXC_OFFLINECACHE_H
