/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2017 Vladimir Svyatski <v.unreal@gmail.com>
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

#include "TestDatabase.h"

#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>

#include "config-keepassx-tests.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Writer.h"
#include "util/TemporaryFile.h"

#ifdef Q_OS_WIN
#include <QFileInfo>
#include <Windows.h>
#endif

QTEST_GUILESS_MAIN(TestDatabase)

static QString dbFileName = QStringLiteral(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx");

void TestDatabase::initTestCase()
{
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());
}

void TestDatabase::testOpen()
{
    auto db = QSharedPointer<Database>::create();
    QVERIFY(!db->isInitialized());
    QVERIFY(!db->isModified());

    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));

    bool ok = db->open(dbFileName, key);
    QVERIFY(ok);

    QVERIFY(db->isInitialized());
    QVERIFY(!db->isModified());

    db->metadata()->setName("test");
    QVERIFY(db->isModified());
}

void TestDatabase::testSave()
{
    TemporaryFile tempFile;
    QVERIFY(tempFile.copyFromFile(dbFileName));

    auto db = QSharedPointer<Database>::create();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));

    QString error;
    bool ok = db->open(tempFile.fileName(), key, &error);
    QVERIFY(ok);

    // Test safe saves
    db->metadata()->setName("test");
    QVERIFY(db->isModified());
    QVERIFY2(db->save(Database::Atomic, {}, &error), error.toLatin1());
    QVERIFY(!db->isModified());

    // Test temp-file saves
    db->metadata()->setName("test2");
    QVERIFY2(db->save(Database::TempFile, QString(), &error), error.toLatin1());
    QVERIFY(!db->isModified());

    // Test direct-write saves
    db->metadata()->setName("test3");
    QVERIFY2(db->save(Database::DirectWrite, QString(), &error), error.toLatin1());
    QVERIFY(!db->isModified());

    // Test save backups
    TemporaryFile backupFile;
    auto backupFilePath = backupFile.fileName();
    db->metadata()->setName("test4");
    QVERIFY2(db->save(Database::Atomic, backupFilePath, &error), error.toLatin1());
    QVERIFY(!db->isModified());

    QVERIFY(QFile::exists(backupFilePath));
    QFile::remove(backupFilePath);
    QVERIFY(!QFile::exists(backupFilePath));
}

void TestDatabase::testSaveAs()
{
    TemporaryFile tempFile;
    QVERIFY(tempFile.copyFromFile(dbFileName));

    auto db = QSharedPointer<Database>::create();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));

    QString error;
    QVERIFY(db->open(tempFile.fileName(), key, &error));

    // Happy path case when try to save as new DB.
    QSignalSpy spyFilePathChanged(db.data(), SIGNAL(filePathChanged(const QString&, const QString&)));
    QString newDbFileName = QStringLiteral(KEEPASSX_TEST_DATA_DIR).append("/SaveAsNewDatabase.kdbx");
    QVERIFY2(db->saveAs(newDbFileName, Database::Atomic, QString(), &error), error.toLatin1());
    QVERIFY(!db->isModified());
    QCOMPARE(spyFilePathChanged.count(), 1);
    QVERIFY(QFile::exists(newDbFileName));
#ifdef Q_OS_WIN
    QVERIFY(!QFileInfo(newDbFileName).isHidden());
    SetFileAttributes(newDbFileName.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
    QVERIFY2(db->saveAs(newDbFileName, Database::Atomic, QString(), &error), error.toLatin1());
    QVERIFY(QFileInfo(newDbFileName).isHidden());
#endif
    QFile::remove(newDbFileName);
    QVERIFY(!QFile::exists(newDbFileName));

    // Negative case when try to save not initialized DB.
    db->releaseData();
    QVERIFY2(!db->saveAs(newDbFileName, Database::Atomic, QString(), &error), error.toLatin1());
    QCOMPARE(error, QString("Could not save, database has not been initialized!"));
}

void TestDatabase::testSignals()
{
    TemporaryFile tempFile;
    QVERIFY(tempFile.copyFromFile(dbFileName));

    auto db = QSharedPointer<Database>::create();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));

    QSignalSpy spyFilePathChanged(db.data(), SIGNAL(filePathChanged(const QString&, const QString&)));
    QString error;
    bool ok = db->open(tempFile.fileName(), key, &error);
    QVERIFY(ok);
    QCOMPARE(spyFilePathChanged.count(), 1);

    QSignalSpy spyModified(db.data(), SIGNAL(modified()));
    db->metadata()->setName("test1");
    QTRY_COMPARE(spyModified.count(), 1);

    QSignalSpy spySaved(db.data(), SIGNAL(databaseSaved()));
    QVERIFY(db->save(Database::Atomic, {}, &error));
    QCOMPARE(spySaved.count(), 1);

    // Short delay to allow file system settling to reduce test failures
    Tools::wait(100);

    QSignalSpy spyFileChanged(db.data(), &Database::databaseFileChanged);
    QVERIFY(tempFile.copyFromFile(dbFileName));
    QTRY_COMPARE(spyFileChanged.count(), 1);
    QTRY_VERIFY(!db->isModified());

    db->metadata()->setName("test2");
    QTRY_VERIFY(db->isModified());

    QSignalSpy spyDiscarded(db.data(), SIGNAL(databaseDiscarded()));
    QVERIFY(db->open(tempFile.fileName(), key, &error));
    QCOMPARE(spyDiscarded.count(), 1);
}

void TestDatabase::testEmptyRecycleBinOnDisabled()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinDisabled.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("123"));
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr));

    QSignalSpy spyModified(db.data(), SIGNAL(modified()));

    db->emptyRecycleBin();
    // The database must be unmodified in this test after emptying the recycle bin.
    QTRY_COMPARE(spyModified.count(), 0);
}

void TestDatabase::testEmptyRecycleBinOnNotCreated()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinNotYetCreated.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("123"));
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr));

    QSignalSpy spyModified(db.data(), SIGNAL(modified()));

    db->emptyRecycleBin();
    // The database must be unmodified in this test after emptying the recycle bin.
    QTRY_COMPARE(spyModified.count(), 0);
}

void TestDatabase::testEmptyRecycleBinOnEmpty()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinEmpty.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("123"));
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr));

    QSignalSpy spyModified(db.data(), SIGNAL(modified()));

    db->emptyRecycleBin();
    // The database must be unmodified in this test after emptying the recycle bin.
    QTRY_COMPARE(spyModified.count(), 0);
}

void TestDatabase::testEmptyRecycleBinWithHierarchicalData()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/RecycleBinWithData.kdbx");
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("123"));
    auto db = QSharedPointer<Database>::create();
    QVERIFY(db->open(filename, key, nullptr));

    QFile originalFile(filename);
    qint64 initialSize = originalFile.size();

    db->emptyRecycleBin();
    QVERIFY(db->metadata()->recycleBin());
    QVERIFY(db->metadata()->recycleBin()->entries().empty());
    QVERIFY(db->metadata()->recycleBin()->children().empty());

    QTemporaryFile afterCleanup;
    afterCleanup.open();

    KeePass2Writer writer;
    writer.writeDatabase(&afterCleanup, db.data());
    QVERIFY(afterCleanup.size() < initialSize);
}

void TestDatabase::testCustomIcons()
{
    Database db;

    QUuid uuid1 = QUuid::createUuid();
    QByteArray icon1("icon 1");
    Q_ASSERT(!icon1.isNull());
    db.metadata()->addCustomIcon(uuid1, icon1);
    Metadata::CustomIconData iconData = db.metadata()->customIcon(uuid1);
    QCOMPARE(iconData.data, icon1);
    QVERIFY(iconData.name.isNull());
    QVERIFY(iconData.lastModified.isNull());

    QUuid uuid2 = QUuid::createUuid();
    QByteArray icon2("icon 2");
    QDateTime date = QDateTime::currentDateTimeUtc();
    db.metadata()->addCustomIcon(uuid2, icon2, "Test", date);
    iconData = db.metadata()->customIcon(uuid2);
    QCOMPARE(iconData.data, icon2);
    QCOMPARE(iconData.name, QString("Test"));
    QCOMPARE(iconData.lastModified, date);
}

void TestDatabase::testExternallyModified()
{
    TemporaryFile tempFile;
    QVERIFY(tempFile.copyFromFile(dbFileName));

    auto db = QSharedPointer<Database>::create();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));

    QString error;
    QVERIFY(db->open(tempFile.fileName(), key, &error) == true);
    db->metadata()->setName("test2");
    QVERIFY(db->save(Database::Atomic, {}, &error));

    QSignalSpy spyFileChanged(db.data(), &Database::databaseFileChanged);
    QVERIFY(tempFile.copyFromFile(dbFileName));
    QTRY_COMPARE(spyFileChanged.count(), 1);
    // the first argument of the databaseFileChanged signal (triggeredBySave) should be false
    QVERIFY(spyFileChanged.at(0).length() == 1);
    QVERIFY(spyFileChanged.at(0).at(0).type() == QVariant::Bool);
    QVERIFY(spyFileChanged.at(0).at(0).toBool() == false);
    spyFileChanged.clear();
    // shouldn't be able to save due to external changes
    QVERIFY(db->save(Database::Atomic, {}, &error) == false);
    QApplication::processEvents();
    // save should have triggered another databaseFileChanged signal
    QVERIFY(spyFileChanged.count() >= 1);
    // the first argument of the databaseFileChanged signal (triggeredBySave) should be true
    QVERIFY(spyFileChanged.at(0).at(0).type() == QVariant::Bool);
    QVERIFY(spyFileChanged.at(0).at(0).toBool() == true);

    // should be able to overwrite externally modified changes when explicitly requested
    db->setIgnoreFileChangesUntilSaved(true);
    QVERIFY(db->save(Database::Atomic, {}, &error));
    // ignoreFileChangesUntilSaved should reset after save
    QVERIFY(db->ignoreFileChangesUntilSaved() == false);
}

void TestDatabase::testCustomAttributeKeys()
{
    Database db;
    db.metadata()->setRecycleBinEnabled(true);
    QVERIFY(db.metadata()->recycleBinEnabled());

    auto root = db.rootGroup();
    QVERIFY(root);

    // Empty database
    db.updateCustomAttributeKeys();
    QCOMPARE(db.customAttributeKeys(), {});

    // Custom attributes of an entry in the root group
    auto entry1 = new Entry();
    entry1->setGroup(root);
    entry1->attributes()->set("Custom10", "value");
    entry1->attributes()->set("Custom2", "value");
    entry1->attributes()->set("DuplicateKey", "value");

    // Custom attributes of an entry in a nested group
    auto group = new Group();
    group->setName("Nested");
    group->setParent(root);

    auto entry2 = new Entry();
    entry2->setGroup(group);
    entry2->attributes()->set("ProtectedKey", "value", true);
    entry2->attributes()->set("DuplicateKey", "value3");
    entry2->attributes()->set("Custom1", "value");
    entry2->attributes()->set("Case-sensitive", "value2"); // Set is case-sensitive

    // Same keys in another entry should only appear once
    auto entry3 = new Entry();
    entry3->setGroup(group);
    entry3->attributes()->set("Custom2", "another value");
    entry3->attributes()->set("case-sensitive", "value"); // Set is case-sensitive
    entry3->attributes()->set("Key with spaces", "test");

    // Entries in the recycle bin do not contribute
    auto recycledEntry = new Entry();
    recycledEntry->setGroup(root);
    recycledEntry->attributes()->set("RecycledKey", "value");
    recycledEntry->attributes()->set("RecycledKeyProtected", "value", true);
    db.recycleEntry(recycledEntry);

    auto groupRecycled = new Group();
    groupRecycled->setName("groupRecycled");
    groupRecycled->setParent(root);
    auto entryInRecycledGroup = new Entry();
    entryInRecycledGroup->setGroup(groupRecycled);
    entryInRecycledGroup->attributes()->set("RecycledKeyGroup", "value");
    db.recycleGroup(groupRecycled);

    // Check if recycledEntry and groupRecycled are actually recycled
    QVERIFY(recycledEntry->isRecycled());
    QVERIFY(groupRecycled->isRecycled());

    db.updateCustomAttributeKeys();

    QVERIFY(!db.customAttributeKeys().contains("RecycledKey"));
    QVERIFY(!db.customAttributeKeys().contains("RecycledKeyProtected"));
    QVERIFY(!db.customAttributeKeys().contains("RecycledKeyGroup"));

    const QStringList expectedKeys = {
        QStringLiteral("Case-sensitive"),
        QStringLiteral("Custom1"),
        QStringLiteral("Custom10"),
        QStringLiteral("Custom2"),
        QStringLiteral("DuplicateKey"),
        QStringLiteral("Key with spaces"),
        QStringLiteral("ProtectedKey"),
        QStringLiteral("case-sensitive"),
    };

    QCOMPARE(db.customAttributeKeys(), expectedKeys);
}
