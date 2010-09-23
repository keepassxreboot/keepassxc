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

#include <QtTest/QTest>

#include "config-keepassx-tests.h"
#include "core/Database.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Reader.h"
#include "keys/PasswordKey.h"

class TestKeePass2Reader : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testNonAscii();
    void testCompressed();
};

void TestKeePass2Reader::initTestCase()
{
    Crypto::init();
}

void TestKeePass2Reader::testNonAscii()
{
    QString filename = QString(KEEPASSX_TEST_DIR).append("/NonAscii.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey(QString::fromUtf8("\xce\x94\xc3\xb6\xd8\xb6")));
    KeePass2Reader* reader = new KeePass2Reader();
    Database* db = reader->readDatabase(filename, key);
    QVERIFY(db);
    QVERIFY(!reader->error());
    QCOMPARE(db->metadata()->name(), QString("NonAsciiTest"));
}

void TestKeePass2Reader::testCompressed()
{
    QString filename = QString(KEEPASSX_TEST_DIR).append("/Compressed.kdbx");
    CompositeKey key;
    key.addKey(PasswordKey(""));
    KeePass2Reader* reader = new KeePass2Reader();
    Database* db = reader->readDatabase(filename, key);
    QVERIFY(db);
    QVERIFY(!reader->error());
    QCOMPARE(db->metadata()->name(), QString("Compressed"));
}

QTEST_MAIN(TestKeePass2Reader);

#include "TestKeePass2Reader.moc"
