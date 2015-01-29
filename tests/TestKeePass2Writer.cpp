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

#include "TestKeePass2Writer.h"

#include <QBuffer>
#include <QTest>

#include "tests.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keys/PasswordKey.h"

QTEST_GUILESS_MAIN(TestKeePass2Writer)

void TestKeePass2Writer::initTestCase()
{
    QVERIFY(Crypto::init());

    CompositeKey key;
    key.addKey(PasswordKey("test"));

    m_dbOrg = new Database();
    m_dbOrg->setKey(key);
    m_dbOrg->metadata()->setName("TESTDB");
    Group* group = m_dbOrg->rootGroup();
    group->setUuid(Uuid::random());
    group->setNotes("I'm a note!");
    Entry* entry = new Entry();
    entry->setPassword(QString::fromUtf8("\xc3\xa4\xa3\xb6\xc3\xbc\xe9\x9b\xbb\xe7\xb4\x85"));
    entry->setUuid(Uuid::random());
    entry->attributes()->set("test", "protectedTest", true);
    QVERIFY(entry->attributes()->isProtected("test"));
    entry->attachments()->set("myattach.txt", QByteArray("this is an attachment"));
    entry->attachments()->set("aaa.txt", QByteArray("also an attachment"));
    entry->setGroup(group);
    Group* groupNew = new Group();
    groupNew->setUuid(Uuid::random());
    groupNew->setName("TESTGROUP");
    groupNew->setNotes("I'm a sub group note!");
    groupNew->setParent(group);

    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);

    KeePass2Writer writer;
    writer.writeDatabase(&buffer, m_dbOrg);
    QVERIFY(!writer.hasError());
    buffer.seek(0);
    KeePass2Reader reader;
    m_dbTest = reader.readDatabase(&buffer, key);
    QVERIFY(!reader.hasError());
    QVERIFY(m_dbTest);
}

void TestKeePass2Writer::testBasic()
{
    QCOMPARE(m_dbTest->metadata()->name(), m_dbOrg->metadata()->name());
    QVERIFY(m_dbTest->rootGroup());
    QCOMPARE(m_dbTest->rootGroup()->children()[0]->name(), m_dbOrg->rootGroup()->children()[0]->name());
    QCOMPARE(m_dbTest->rootGroup()->notes(), m_dbOrg->rootGroup()->notes());
    QCOMPARE(m_dbTest->rootGroup()->children()[0]->notes(), m_dbOrg->rootGroup()->children()[0]->notes());
}

void TestKeePass2Writer::testProtectedAttributes()
{
    QCOMPARE(m_dbTest->rootGroup()->entries().size(), 1);
    Entry* entry = m_dbTest->rootGroup()->entries().at(0);
    QCOMPARE(entry->attributes()->value("test"), QString("protectedTest"));
    QCOMPARE(entry->attributes()->isProtected("test"), true);
}

void TestKeePass2Writer::testAttachments()
{
    Entry* entry = m_dbTest->rootGroup()->entries().at(0);
    QCOMPARE(entry->attachments()->keys().size(), 2);
    QCOMPARE(entry->attachments()->value("myattach.txt"), QByteArray("this is an attachment"));
    QCOMPARE(entry->attachments()->value("aaa.txt"), QByteArray("also an attachment"));
}

void TestKeePass2Writer::testNonAsciiPasswords()
{
    QCOMPARE(m_dbTest->rootGroup()->entries()[0]->password(), m_dbOrg->rootGroup()->entries()[0]->password());
}

void TestKeePass2Writer::cleanupTestCase()
{
    delete m_dbOrg;
    delete m_dbTest;
}
