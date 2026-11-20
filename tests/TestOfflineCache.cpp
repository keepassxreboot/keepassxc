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

#include "TestOfflineCache.h"

#include "core/Config.h"
#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/OfflineCache.h"
#include "crypto/Crypto.h"
#include "format/KeePass2.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

QTEST_GUILESS_MAIN(TestOfflineCache)

namespace
{
    QString writeFile(const QTemporaryDir& dir, const QString& name, const QByteArray& content)
    {
        const auto path = dir.filePath(name);
        QFile file(path);
        file.open(QIODevice::WriteOnly);
        file.write(content);
        return path;
    }

    QByteArray readFile(const QString& path)
    {
        QFile file(path);
        file.open(QIODevice::ReadOnly);
        return file.readAll();
    }

    OfflineCache::Fingerprints fp(const QString& content, const QString& structure = "s1")
    {
        return {content, structure};
    }

    Entry* addEntry(Database* db, const QString& password, const QString& title = {})
    {
        auto* entry = new Entry();
        entry->setUuid(QUuid::createUuid());
        entry->setGroup(db->rootGroup());
        entry->setPassword(password);
        if (!title.isEmpty()) {
            entry->setTitle(title);
        }
        return entry;
    }

    QSharedPointer<CompositeKey> makePasswordKey(const QString& password)
    {
        auto key = QSharedPointer<CompositeKey>::create();
        key->addKey(QSharedPointer<PasswordKey>::create(password));
        return key;
    }

    bool hasEntryWithTitle(const Database* db, const QString& title)
    {
        const auto entries = db->rootGroup()->entriesRecursive();
        for (const Entry* entry : entries) {
            if (entry->title() == title) {
                return true;
            }
        }
        return false;
    }
} // namespace

void TestOfflineCache::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestOfflineCache::testStoreCopyAndDirtyFlag()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto original = writeFile(dir, "db.kdbx", "original-bytes");
    OfflineCache cache(original, dir.filePath("cache"));

    QVERIFY(!cache.hasCacheFile());
    QVERIFY(!cache.isDirty());

    QVERIFY(cache.storeCopyOf(original));
    QVERIFY(cache.hasCacheFile());
    QCOMPARE(readFile(cache.cacheFilePath()), QByteArray("original-bytes"));

    // the copy is replaced as a whole
    writeFile(dir, "db.kdbx", "newer");
    QVERIFY(cache.storeCopyOf(original));
    QCOMPARE(readFile(cache.cacheFilePath()), QByteArray("newer"));

    QVERIFY(cache.setDirty(true));
    QVERIFY(cache.isDirty());
    // a new instance for the same file sees the same state (survives restarts)
    OfflineCache again(original, dir.filePath("cache"));
    QCOMPARE(again.cacheFilePath(), cache.cacheFilePath());
    QVERIFY(again.isDirty());
    QVERIFY(again.setDirty(false));
    QVERIFY(!cache.isDirty());

    // a missing source must fail without touching the cache
    QVERIFY(!cache.storeCopyOf(dir.filePath("missing.kdbx")));
    QCOMPARE(readFile(cache.cacheFilePath()), QByteArray("newer"));
}

void TestOfflineCache::testFingerprintIgnoresVolatileData()
{
    Database db;
    auto* entry = addEntry(&db, "secret");
    const auto before = OfflineCache::fingerprint(&db);
    QVERIFY(!before.content.isEmpty());
    QVERIFY(!before.structure.isEmpty());

    // usage count, last access time and the random slug written on every save must not matter
    auto timeInfo = entry->timeInfo();
    timeInfo.setUsageCount(timeInfo.usageCount() + 5);
    timeInfo.setLastAccessTime(timeInfo.lastAccessTime().addDays(1));
    entry->setTimeInfo(timeInfo);
    db.metadata()->customData()->set(CustomData::RandomSlug, "abcdef");

    const auto after = OfflineCache::fingerprint(&db);
    QCOMPARE(after.content, before.content);
    QCOMPARE(after.structure, before.structure);
}

void TestOfflineCache::testFingerprintDetectsChanges()
{
    Database db;
    auto* entry = addEntry(&db, "secret");
    const auto base = OfflineCache::fingerprint(&db);

    // password change: content differs, structure stays
    entry->setPassword("other");
    const auto changed = OfflineCache::fingerprint(&db);
    QVERIFY(changed.content != base.content);
    QCOMPARE(changed.structure, base.structure);

    // entry added: both differ
    addEntry(&db, "another");
    const auto added = OfflineCache::fingerprint(&db);
    QVERIFY(added.content != changed.content);
    QVERIFY(added.structure != changed.structure);

    // entry deleted: structure differs from the state with the entry
    delete entry;
    const auto deleted = OfflineCache::fingerprint(&db);
    QVERIFY(deleted.structure != added.structure);
    QVERIFY(deleted.content != added.content);

    // master key changed
    db.metadata()->setDatabaseKeyChanged(db.metadata()->databaseKeyChanged().addSecs(60));
    QVERIFY(OfflineCache::fingerprint(&db).structure != deleted.structure);
}

void TestOfflineCache::testVersionCoalescing()
{
    QTemporaryDir dir;
    const auto src = writeFile(dir, "src.kdbx", "v1");
    OfflineCache cache(dir.filePath("db.kdbx"), dir.filePath("cache"));
    const QDateTime t0(QDate(2026, 1, 1), QTime(10, 0));

    QVERIFY(cache.addVersion(src, fp("c1"), false, t0));
    QCOMPARE(cache.versions().size(), 1);

    // identical content: nothing happens
    QVERIFY(cache.addVersion(src, fp("c1"), false, t0.addSecs(30)));
    QCOMPARE(cache.versions().size(), 1);

    // changed content (same structure) inside the window: newest slot is overwritten
    writeFile(dir, "src.kdbx", "v2");
    QVERIFY(cache.addVersion(src, fp("c2"), false, t0.addSecs(5 * 60)));
    QCOMPARE(cache.versions().size(), 1);
    const auto first = cache.versions().first();
    QCOMPARE(readFile(QDir(cache.versionsDirectory()).filePath(first.fileName)), QByteArray("v2"));
    QCOMPARE(first.fingerprints.content, QString("c2"));

    // the window is measured from the slot start, not from the last overwrite
    writeFile(dir, "src.kdbx", "v3");
    QVERIFY(cache.addVersion(src, fp("c3"), false, t0.addSecs(11 * 60)));
    QCOMPARE(cache.versions().size(), 2);
    QCOMPARE(readFile(QDir(cache.versionsDirectory()).filePath(cache.versions().first().fileName)),
             QByteArray("v2"));
}

void TestOfflineCache::testStructuralChangeAlwaysNewVersion()
{
    QTemporaryDir dir;
    const auto src = writeFile(dir, "src.kdbx", "v1");
    OfflineCache cache(dir.filePath("db.kdbx"), dir.filePath("cache"));
    const QDateTime t0(QDate(2026, 1, 1), QTime(10, 0));

    QVERIFY(cache.addVersion(src, fp("c1", "s1"), false, t0));
    // entry added one minute later: must not be coalesced
    writeFile(dir, "src.kdbx", "v2");
    QVERIFY(cache.addVersion(src, fp("c2", "s2"), false, t0.addSecs(60)));
    QCOMPARE(cache.versions().size(), 2);
    QVERIFY(cache.versions().last().structural);

    // a content-only change right after a structural one gets its own slot
    // and the structural slot is never overwritten
    writeFile(dir, "src.kdbx", "v3");
    QVERIFY(cache.addVersion(src, fp("c3", "s2"), false, t0.addSecs(120)));
    QCOMPARE(cache.versions().size(), 3);
    QCOMPARE(readFile(QDir(cache.versionsDirectory()).filePath(cache.versions().at(1).fileName)),
             QByteArray("v2"));
}

void TestOfflineCache::testForceNewVersion()
{
    QTemporaryDir dir;
    const auto src = writeFile(dir, "src.kdbx", "v1");
    OfflineCache cache(dir.filePath("db.kdbx"), dir.filePath("cache"));
    const QDateTime t0(QDate(2026, 1, 1), QTime(10, 0));

    QVERIFY(cache.addVersion(src, fp("c1"), false, t0));
    // explicit snapshot (before a merge) even though the content is identical and inside the window
    QVERIFY(cache.addVersion(src, fp("c1"), true, t0.addSecs(10)));
    QCOMPARE(cache.versions().size(), 2);
    // same timestamp must not clash on file names
    QVERIFY(cache.addVersion(src, fp("c1"), true, t0.addSecs(10)));
    QCOMPARE(cache.versions().size(), 3);
    QCOMPARE(QDir(cache.versionsDirectory()).entryList(QDir::Files).size(), 3);
}

void TestOfflineCache::testRotation()
{
    QTemporaryDir dir;
    const auto src = writeFile(dir, "src.kdbx", "v");
    OfflineCache cache(dir.filePath("db.kdbx"), dir.filePath("cache"));
    QCOMPARE(cache.maxVersions(), 10);
    const QDateTime t0(QDate(2026, 1, 1), QTime(10, 0));

    for (int i = 0; i < 15; ++i) {
        // every call adds a new entry (structural change)
        QVERIFY(cache.addVersion(src, fp(QString("c%1").arg(i), QString("s%1").arg(i)), false, t0.addSecs(i * 60)));
    }
    const auto versions = cache.versions();
    QCOMPARE(versions.size(), 10);
    QCOMPARE(versions.first().fingerprints.content, QString("c5"));
    QCOMPARE(versions.last().fingerprints.content, QString("c14"));
    QCOMPARE(QDir(cache.versionsDirectory()).entryList(QDir::Files).size(), 10);

    cache.setMaxVersions(3);
    QVERIFY(cache.addVersion(src, fp("c15", "s15"), false, t0.addSecs(16 * 60)));
    QCOMPARE(cache.versions().size(), 3);
    QCOMPARE(QDir(cache.versionsDirectory()).entryList(QDir::Files).size(), 3);
}

void TestOfflineCache::testRemoveAll()
{
    QTemporaryDir dir;
    const auto src = writeFile(dir, "src.kdbx", "v");
    OfflineCache cache(dir.filePath("db.kdbx"), dir.filePath("cache"));
    QVERIFY(cache.storeCopyOf(src));
    QVERIFY(cache.addVersion(src, fp("c1")));
    QVERIFY(cache.removeAll());
    QVERIFY(!cache.hasCacheFile());
    QVERIFY(cache.versions().isEmpty());
}

void TestOfflineCache::testDatabaseFallbackOpenAndOfflineSave()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    config()->set(Config::OfflineCacheDir, dir.filePath("cache-root"));

    const QString cloudDir = dir.filePath("pcloud");
    QVERIFY(QDir().mkpath(cloudDir));
    const QString cloudPath = QDir(cloudDir).filePath("secrets.kdbx");
    const auto key = makePasswordKey("master-pass-123");

    // 1. Create a cloud-enabled database while online and save it
    {
        Database db;
        QVERIFY(db.setKey(key));
        db.metadata()->customData()->set(CustomData::CloudFile, QStringLiteral("true"));
        addEntry(&db, "pw1", "entry-initial");
        QString error;
        QVERIFY2(db.saveAs(cloudPath, Database::Atomic, {}, &error), qPrintable(error));
        QVERIFY(!db.isWorkingOffline());
        QVERIFY(db.isCloudModeEnabled());
    }

    OfflineCache cache(cloudPath);
    QVERIFY(cache.hasCacheFile());
    QVERIFY(!cache.isDirty());
    QVERIFY(cache.isSuggestedByDatabase());

    // 2. Simulate pCloud drive going offline (remove the entire cloud directory)
    QVERIFY(QDir(cloudDir).removeRecursively());
    QVERIFY(!QFileInfo::exists(cloudPath));
    QVERIFY(OfflineCache::canFallbackToCache(cloudPath));

    // 3. Open the unreachable cloud path (simulating app restart while offline)
    {
        Database offlineDb;
        QString error;
        QVERIFY2(offlineDb.open(cloudPath, key, &error), qPrintable(error));
        QVERIFY(offlineDb.isWorkingOffline());
        QCOMPARE(OfflineCache::normalizeFilePath(offlineDb.filePath()), OfflineCache::normalizeFilePath(cloudPath));
        QVERIFY(hasEntryWithTitle(&offlineDb, "entry-initial"));

        // 4. Add a new entry while offline and save (parent folder of cloudPath is still missing!)
        addEntry(&offlineDb, "pw2", "entry-added-offline");
        QVERIFY2(offlineDb.save(Database::Atomic, {}, &error), qPrintable(error));
        QVERIFY(offlineDb.isWorkingOffline());
        QVERIFY(cache.isDirty());
        QVERIFY(!cache.versions().isEmpty());
    }

    // 5. Simulate a second app restart while still offline: verify the new offline entry is preserved
    {
        Database offlineRestartDb;
        QString error;
        QVERIFY2(offlineRestartDb.open(cloudPath, key, &error), qPrintable(error));
        QVERIFY(offlineRestartDb.isWorkingOffline());
        QVERIFY(hasEntryWithTitle(&offlineRestartDb, "entry-initial"));
        QVERIFY(hasEntryWithTitle(&offlineRestartDb, "entry-added-offline"));
    }
}

void TestOfflineCache::testFingerprintAndFallbackAcrossFileVersions()
{
    const QList<quint32> versions = {
        KeePass2::FILE_VERSION_3_1,
        KeePass2::FILE_VERSION_4,
        KeePass2::FILE_VERSION_4_1,
    };

    for (const quint32 version : versions) {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        config()->set(Config::OfflineCacheDir, dir.filePath("cache-root"));

        const QString dbPath = dir.filePath(QString("ver-%1.kdbx").arg(version, 0, 16));
        const auto key = makePasswordKey("version-test-pass");

        // Verify fingerprint() works without asserting and detects changes on this format version
        {
            Database db;
            db.setFormatVersion(version);
            QVERIFY(db.setKey(key));
            auto* entry = addEntry(&db, "pw-initial", "entry-1");
            entry->attachments()->set("note.txt", QByteArray("hello"));

            const auto fp1 = OfflineCache::fingerprint(&db);
            QVERIFY(!fp1.content.isEmpty());
            QVERIFY(!fp1.structure.isEmpty());

            // Volatile fields must still be ignored
            auto timeInfo = entry->timeInfo();
            timeInfo.setUsageCount(timeInfo.usageCount() + 1);
            timeInfo.setLastAccessTime(timeInfo.lastAccessTime().addSecs(120));
            entry->setTimeInfo(timeInfo);
            db.metadata()->customData()->set(CustomData::RandomSlug, "slug");
            QCOMPARE(OfflineCache::fingerprint(&db).content, fp1.content);

            // Password change must alter content fingerprint only
            entry->setPassword("pw-changed");
            const auto fp2 = OfflineCache::fingerprint(&db);
            QVERIFY(fp2.content != fp1.content);
            QCOMPARE(fp2.structure, fp1.structure);

            // Enable cloud mode via local override (keeps KDBX 3.1 as 3.1 when no KDBX 4 features are used)
            OfflineCache::setLocalOverride(dbPath, OfflineCache::Override::Enabled);
            if (version >= KeePass2::FILE_VERSION_4) {
                db.metadata()->customData()->set(CustomData::CloudFile, QStringLiteral("true"));
            }
            if (version >= KeePass2::FILE_VERSION_4_1) {
                entry->setExcludeFromReports(true);
            }

            QString error;
            QVERIFY2(db.saveAs(dbPath, Database::Atomic, {}, &error), qPrintable(error));
            QVERIFY(db.formatVersion() >= version);
        }

        // Simulate offline: remove original, open from cache, edit, save, restore, and reconcile
        const QString backupCopy = dir.filePath("remote.kdbx");
        QVERIFY(QFile::copy(dbPath, backupCopy));
        QVERIFY(QFile::remove(dbPath));

        {
            Database offlineDb;
            QString error;
            QVERIFY2(offlineDb.open(dbPath, key, &error), qPrintable(error));
            QVERIFY(offlineDb.isWorkingOffline());
            QVERIFY(hasEntryWithTitle(&offlineDb, "entry-1"));

            addEntry(&offlineDb, "pw-offline", "entry-offline");
            QVERIFY2(offlineDb.save(Database::Atomic, {}, &error), qPrintable(error));

            QVERIFY(QFile::copy(backupCopy, dbPath));
            QVERIFY2(offlineDb.reconcileOfflineCache(nullptr, &error), qPrintable(error));
            QVERIFY(!offlineDb.isWorkingOffline());
            QVERIFY(hasEntryWithTitle(&offlineDb, "entry-1"));
            QVERIFY(hasEntryWithTitle(&offlineDb, "entry-offline"));
        }

        OfflineCache::setLocalOverride(dbPath, OfflineCache::Override::Unset);
    }
}

void TestOfflineCache::testReconnectMergeNeverLosesNewItems()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    config()->set(Config::OfflineCacheDir, dir.filePath("cache-root"));

    const QString cloudPath = dir.filePath("shared.kdbx");
    const auto key = makePasswordKey("merge-pass-123");

    // 1. Create initial cloud database
    Database initialDb;
    QVERIFY(initialDb.setKey(key));
    initialDb.metadata()->customData()->set(CustomData::CloudFile, QStringLiteral("true"));
    addEntry(&initialDb, "pw-init", "entry-initial");
    QString error;
    QVERIFY2(initialDb.saveAs(cloudPath, Database::Atomic, {}, &error), qPrintable(error));

    // Save a copy of the base file so we can simulate another computer modifying it
    const QString remoteTempPath = dir.filePath("remote-device.kdbx");
    QVERIFY(QFile::copy(cloudPath, remoteTempPath));

    // 2. Go offline and add an entry locally
    QVERIFY(QFile::remove(cloudPath));
    Database offlineDb;
    QVERIFY2(offlineDb.open(cloudPath, key, &error), qPrintable(error));
    QVERIFY(offlineDb.isWorkingOffline());
    QTest::qWait(1100); // Ensure distinct modification timestamps
    addEntry(&offlineDb, "pw-local", "entry-added-offline");
    QVERIFY2(offlineDb.save(Database::Atomic, {}, &error), qPrintable(error));

    OfflineCache cache(cloudPath);
    QVERIFY(cache.isDirty());

    // 3. Meanwhile, on "another device", open the base database, add a remote entry, and sync it to cloudPath
    {
        Database remoteDb;
        QVERIFY2(remoteDb.open(remoteTempPath, key, &error), qPrintable(error));
        addEntry(&remoteDb, "pw-remote", "entry-added-remotely");
        QVERIFY2(remoteDb.save(Database::Atomic, {}, &error), qPrintable(error));
    }
    QVERIFY(QFile::copy(remoteTempPath, cloudPath));

    // 4. Reconcile the live offlineDb when cloudPath comes back online
    QVERIFY2(offlineDb.reconcileOfflineCache(nullptr, &error), qPrintable(error));
    QVERIFY(!offlineDb.isWorkingOffline());
    QVERIFY(!cache.isDirty());
    QVERIFY(hasEntryWithTitle(&offlineDb, "entry-initial"));
    QVERIFY(hasEntryWithTitle(&offlineDb, "entry-added-offline"));
    QVERIFY(hasEntryWithTitle(&offlineDb, "entry-added-remotely"));
    QVERIFY(cache.versions().size() >= 2);

    // 5. Also verify opening from scratch when cache is dirty and cloudPath has concurrent changes
    QVERIFY(QFile::remove(remoteTempPath));
    QVERIFY(QFile::copy(cloudPath, remoteTempPath));
    QVERIFY(QFile::remove(cloudPath));

    Database offlineDb2;
    QVERIFY2(offlineDb2.open(cloudPath, key, &error), qPrintable(error));
    QVERIFY(offlineDb2.isWorkingOffline());
    QTest::qWait(1100);
    addEntry(&offlineDb2, "pw-local-2", "entry-added-offline-2");
    QVERIFY2(offlineDb2.save(Database::Atomic, {}, &error), qPrintable(error));
    QVERIFY(cache.isDirty());

    // Simulate another remote entry added on another device while KeePassXC was closed
    {
        Database remoteDb2;
        QVERIFY2(remoteDb2.open(remoteTempPath, key, &error), qPrintable(error));
        addEntry(&remoteDb2, "pw-remote-2", "entry-added-remotely-2");
        QVERIFY2(remoteDb2.save(Database::Atomic, {}, &error), qPrintable(error));
    }
    QVERIFY(QFile::copy(remoteTempPath, cloudPath));

    Database startupMergedDb;
    QVERIFY2(startupMergedDb.open(cloudPath, key, &error), qPrintable(error));
    QVERIFY(!startupMergedDb.isWorkingOffline());
    QVERIFY(!cache.isDirty());
    QVERIFY(hasEntryWithTitle(&startupMergedDb, "entry-initial"));
    QVERIFY(hasEntryWithTitle(&startupMergedDb, "entry-added-offline"));
    QVERIFY(hasEntryWithTitle(&startupMergedDb, "entry-added-offline-2"));
    QVERIFY(hasEntryWithTitle(&startupMergedDb, "entry-added-remotely"));
    QVERIFY(hasEntryWithTitle(&startupMergedDb, "entry-added-remotely-2"));
}

void TestOfflineCache::testReconnectKeyMismatchPreservesOfflineCopy()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    config()->set(Config::OfflineCacheDir, dir.filePath("cache-root"));

    const QString cloudPath = dir.filePath("keychange.kdbx");
    const auto oldKey = makePasswordKey("old-password");
    const auto newKey = makePasswordKey("new-password");

    // 1. Create cloud database with oldKey
    {
        Database db;
        QVERIFY(db.setKey(oldKey));
        db.metadata()->customData()->set(CustomData::CloudFile, QStringLiteral("true"));
        addEntry(&db, "pw1", "entry-initial");
        QString error;
        QVERIFY2(db.saveAs(cloudPath, Database::Atomic, {}, &error), qPrintable(error));
    }

    const QString remoteTempPath = dir.filePath("remote-rekeyed.kdbx");
    QVERIFY(QFile::copy(cloudPath, remoteTempPath));

    // 2. Go offline and make a local edit encrypted with oldKey
    QVERIFY(QFile::remove(cloudPath));
    {
        Database offlineDb;
        QString error;
        QVERIFY2(offlineDb.open(cloudPath, oldKey, &error), qPrintable(error));
        addEntry(&offlineDb, "pw-offline", "entry-offline-unsynced");
        QVERIFY2(offlineDb.save(Database::Atomic, {}, &error), qPrintable(error));
    }

    OfflineCache cache(cloudPath);
    QVERIFY(cache.isDirty());

    // 3. Re-key remoteTempPath with newKey on "another computer" and sync raw file to cloudPath
    {
        Database remoteDb;
        QString error;
        QVERIFY2(remoteDb.open(remoteTempPath, oldKey, &error), qPrintable(error));
        QVERIFY(remoteDb.setKey(newKey));
        QVERIFY2(remoteDb.save(Database::Atomic, {}, &error), qPrintable(error));
    }
    QVERIFY(QFile::copy(remoteTempPath, cloudPath));

    // 4. Open cloudPath with newKey: must succeed, flag manual merge, and preserve the dirty offline copy
    Database reopenedDb;
    QString error;
    QVERIFY2(reopenedDb.open(cloudPath, newKey, &error), qPrintable(error));
    QVERIFY(reopenedDb.offlineCacheNeedsManualMerge());
    QVERIFY(cache.hasCacheFile());
    QVERIFY(cache.isDirty());
}
