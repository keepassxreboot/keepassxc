/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "TestHibp.h"

#include "core/Group.h"
#include "core/HibpOffline.h"
#include "crypto/Crypto.h"

#include <QBuffer>
#include <QByteArray>
#include <QList>
#include <QTest>

QTEST_GUILESS_MAIN(TestHibp)

const char* TEST_HIBP_CONTENTS = "0BEEC7B5EA3F0FDBC95D0DD47F3C5BC275DA8A33:123\n" // SHA-1 of "foo"
                                 "62cdb7020ff920e5aa642c3d4066950dd1f01f4d:456\n"; // SHA-1 of "bar"

const char* TEST_BAD_HIBP_CONTENTS = "barf:nope\n";

void TestHibp::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestHibp::init()
{
    m_db.reset(new Database());
}

void TestHibp::testBadHibpFormat()
{
    QByteArray hibpContents(TEST_BAD_HIBP_CONTENTS);
    QBuffer hibpBuffer(&hibpContents);
    QVERIFY(hibpBuffer.open(QIODevice::ReadOnly));

    QList<QPair<const Entry*, int>> findings;
    QString error;
    QVERIFY(!HibpOffline::report(m_db, hibpBuffer, findings, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(findings.size(), 0);
}

void TestHibp::testEmpty()
{
    QByteArray hibpContents(TEST_HIBP_CONTENTS);
    QBuffer hibpBuffer(&hibpContents);
    QVERIFY(hibpBuffer.open(QIODevice::ReadOnly));

    QList<QPair<const Entry*, int>> findings;
    QString error;
    QVERIFY(HibpOffline::report(m_db, hibpBuffer, findings, &error));
    QCOMPARE(error, QString());
    QCOMPARE(findings.size(), 0);
}

void TestHibp::testIoError()
{
    QBuffer hibpBuffer;
    // hibpBuffer has not been opened, so reading will cause I/O error

    QList<QPair<const Entry*, int>> findings;
    QString error;
    QVERIFY(!HibpOffline::report(m_db, hibpBuffer, findings, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(findings.size(), 0);
}

void TestHibp::testPwned()
{
    QByteArray hibpContents(TEST_HIBP_CONTENTS);
    QBuffer hibpBuffer(&hibpContents);
    QVERIFY(hibpBuffer.open(QIODevice::ReadOnly));

    Group* root = m_db->rootGroup();

    auto entry1 = new Entry();
    entry1->setPassword("foo");
    entry1->setGroup(root);

    auto entry2 = new Entry();
    entry2->setPassword("xyz");
    entry2->setGroup(root);

    auto entry3 = new Entry();
    entry3->setPassword("foo");
    m_db->recycleEntry(entry3);

    auto group1 = new Group();
    group1->setParent(root);

    auto entry4 = new Entry();
    entry4->setPassword("bar");
    entry4->setGroup(group1);

    QList<QPair<const Entry*, int>> findings;
    QString error;
    QVERIFY(HibpOffline::report(m_db, hibpBuffer, findings, &error));
    QCOMPARE(error, QString());
    QCOMPARE(findings.size(), 2);
    QCOMPARE(findings[0].first, entry1);
    QCOMPARE(findings[0].second, 123);
    QCOMPARE(findings[1].first, entry4);
    QCOMPARE(findings[1].second, 456);
}
