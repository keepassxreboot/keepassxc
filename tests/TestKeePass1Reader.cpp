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

#include "TestKeePass1Reader.h"

#include <QtTest/QTest>

#include "config-keepassx-tests.h"
#include "tests.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "crypto/Crypto.h"
#include "format/KeePass1Reader.h"

void TestKeePass1Reader::initTestCase()
{
    Crypto::init();
}

void TestKeePass1Reader::testBasic()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/basic.kdb");

    KeePass1Reader reader;
    Database* db = reader.readDatabase(filename, "masterpw", QByteArray());
    QVERIFY(db);
    QVERIFY(!reader.hasError());

    QCOMPARE(db->rootGroup()->children().size(), 2);

    Group* group1 = db->rootGroup()->children().at(0);
    QCOMPARE(group1->name(), QString("Internet"));
    QCOMPARE(group1->iconNumber(), 1);
    QCOMPARE(group1->entries().size(), 2);

    Entry* entry11 = group1->entries().at(0);
    QCOMPARE(entry11->title(), QString("Test entry"));
    QCOMPARE(entry11->iconNumber(), 1);
    QCOMPARE(entry11->username(), QString("I"));
    QCOMPARE(entry11->url(), QString("http://example.com/"));
    QCOMPARE(entry11->password(), QString("secretpassword"));
    QCOMPARE(entry11->notes(), QString("Lorem ipsum\ndolor sit amet"));
    QVERIFY(entry11->timeInfo().expires());
    QCOMPARE(entry11->timeInfo().expiryTime(), genDT(2012, 5, 9, 10, 32));
    QCOMPARE(entry11->attachments()->keys().size(), 1);
    QCOMPARE(entry11->attachments()->keys().first(), QString("attachment.txt"));
    QCOMPARE(entry11->attachments()->value("attachment.txt"), QByteArray("hello world\n"));

    Entry* entry12 = group1->entries().at(1);
    QCOMPARE(entry12->title(), QString(""));
    QCOMPARE(entry12->iconNumber(), 0);
    QCOMPARE(entry12->username(), QString(""));
    QCOMPARE(entry12->url(), QString(""));
    QCOMPARE(entry12->password(), QString(""));
    QCOMPARE(entry12->notes(), QString(""));
    QVERIFY(!entry12->timeInfo().expires());
    QCOMPARE(entry12->attachments()->keys().size(), 0);

    Group* group2 = db->rootGroup()->children().at(1);
    QCOMPARE(group2->name(), QString("eMail"));
    QCOMPARE(group2->iconNumber(), 19);
    QCOMPARE(group2->entries().size(), 0);

    delete db;
}

QDateTime TestKeePass1Reader::genDT(int year, int month, int day, int hour, int min)
{
    QDate date(year, month, day);
    QTime time(hour, min, 0);
    return QDateTime(date, time, Qt::UTC);
}

KEEPASSX_QTEST_CORE_MAIN(TestKeePass1Reader)
