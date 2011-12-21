/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "TestKeys.h"

#include <QtCore/QBuffer>
#include <QtTest/QTest>

#include "config-keepassx-tests.h"
#include "core/Database.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keys/CompositeKey.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

void TestKeys::initTestCase()
{
    Crypto::init();
}

void TestKeys::testComposite()
{
    CompositeKey* compositeKey1 = new CompositeKey();
    PasswordKey* passwordKey1 = new PasswordKey();
    PasswordKey* passwordKey2 = new PasswordKey("test");

    compositeKey1->addKey(*passwordKey1);
    compositeKey1->addKey(*passwordKey2);
    delete passwordKey1;
    delete passwordKey2;

    QVERIFY(compositeKey1->transform(QByteArray(32, '\0'), 1).size() == 32);

    // make sure the subkeys are copied
    CompositeKey* compositeKey2 = compositeKey1->clone();
    delete compositeKey1;

    QVERIFY(compositeKey2->transform(QByteArray(32, '\0'), 1).size() == 32);

    delete compositeKey2;
}

void TestKeys::testFileKey()
{
    QFETCH(QString, type);

    QString name = QString("FileKey").append(type);

    KeePass2Reader reader;

    QString dbFilename = QString("%1/%2.kdbx").arg(QString(KEEPASSX_TEST_DATA_DIR), name);
    QString keyFilename = QString("%1/%2.key").arg(QString(KEEPASSX_TEST_DATA_DIR), name);

    CompositeKey compositeKey;
    FileKey fileKey;
    QVERIFY(fileKey.load(keyFilename));
    QCOMPARE(fileKey.rawKey().size(), 32);
    compositeKey.addKey(fileKey);

    Database* db = reader.readDatabase(dbFilename, compositeKey);
    QVERIFY(db);
    QVERIFY(!reader.error());
    QCOMPARE(db->metadata()->name(), QString("%1 Database").arg(name));

    delete db;
}

void TestKeys::testFileKey_data()
{
    QTest::addColumn<QString>("type");
    QTest::newRow("Xml") << QString("Xml");
    QTest::newRow("Binary") << QString("Binary");
    QTest::newRow("Hex") << QString("Hex");
    QTest::newRow("Hashed") << QString("Hashed");
}

void TestKeys::testCreateFileKey()
{
    const QString dbName("testCreateFileKey database");

    QBuffer keyBuffer;
    keyBuffer.open(QBuffer::ReadWrite);

    FileKey::create(&keyBuffer);
    keyBuffer.reset();

    FileKey fileKey;
    QVERIFY(fileKey.load(&keyBuffer));
    CompositeKey compositeKey;
    compositeKey.addKey(fileKey);

    Database* dbOrg = new Database();
    dbOrg->setKey(compositeKey);
    dbOrg->metadata()->setName(dbName);

    QBuffer dbBuffer;
    dbBuffer.open(QBuffer::ReadWrite);

    KeePass2Writer writer;
    writer.writeDatabase(&dbBuffer, dbOrg);
    dbBuffer.reset();
    delete dbOrg;

    KeePass2Reader reader;
    Database* dbRead = reader.readDatabase(&dbBuffer, compositeKey);
    QVERIFY(dbRead);
    QVERIFY(!reader.error());
    QCOMPARE(dbRead->metadata()->name(), dbName);
    delete dbRead;
}

void TestKeys::testFileKeyError()
{
    bool result;
    QString errorMsg;
    const QString fileName(QString(KEEPASSX_TEST_DATA_DIR).append("/does/not/exist"));

    FileKey fileKey;
    result = fileKey.load(fileName, &errorMsg);
    QVERIFY(!result);
    QVERIFY(!errorMsg.isEmpty());
    errorMsg = "";

    result = FileKey::create(fileName, &errorMsg);
    QVERIFY(!result);
    QVERIFY(!errorMsg.isEmpty());
    errorMsg = "";
}

QTEST_MAIN(TestKeys);
