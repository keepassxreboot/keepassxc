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

#include "core/Database.h"
#include "core/Metadata.h"
#include "core/Parser.h"
#include "config-keepassx-tests.h"

class TestParser : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testMetadata();
    void testGroups();

private:
    QDateTime genDT(int year, int month, int day, int hour, int min, int second);

    Database* m_db;
};

QDateTime TestParser::genDT(int year, int month, int day, int hour, int min, int second)
{
    QDate date(year, month, day);
    QTime time(hour, min, second);
    return QDateTime(date, time, Qt::UTC);
}

void TestParser::initTestCase()
{
    m_db = new Database();
    Parser* parser = new Parser(m_db);
    QString xmlFile = QString(KEEPASSX_TEST_DIR).append("/NewDatabase.xml");
    QVERIFY(parser->parse(xmlFile));
}

void TestParser::testMetadata()
{
    QCOMPARE(m_db->metadata()->generator(), QString("KeePass"));
    QCOMPARE(m_db->metadata()->name(), QString("ANAME"));
    QCOMPARE(m_db->metadata()->nameChanged(), genDT(2010, 8, 8, 17, 24, 53));
    QCOMPARE(m_db->metadata()->description(), QString("ADESC"));
    QCOMPARE(m_db->metadata()->descriptionChanged(), genDT(2010, 8, 8, 17, 27, 12));
    QCOMPARE(m_db->metadata()->defaultUserName(), QString("DEFUSERNAME"));
    QCOMPARE(m_db->metadata()->defaultUserNameChanged(), genDT(2010, 8, 8, 17, 27, 45));
    QCOMPARE(m_db->metadata()->maintenanceHistoryDays(), 127);
    QCOMPARE(m_db->metadata()->protectTitle(), false);
    QCOMPARE(m_db->metadata()->protectUsername(), true);
    QCOMPARE(m_db->metadata()->protectPassword(), false);
    QCOMPARE(m_db->metadata()->protectUrl(), true);
    QCOMPARE(m_db->metadata()->protectNotes(), false);
    QCOMPARE(m_db->metadata()->autoEnableVisualHiding(), true);
    QCOMPARE(m_db->metadata()->recycleBinEnabled(), true);
    QVERIFY(m_db->metadata()->recycleBin() != 0);
    QCOMPARE(m_db->metadata()->recycleBin()->name(), QString("Recycle Bin"));
    QCOMPARE(m_db->metadata()->recycleBinChanged(), genDT(2010, 8, 8, 17, 24, 17));
    QVERIFY(m_db->metadata()->entryTemplatesGroup() == 0);
    QCOMPARE(m_db->metadata()->entryTemplatesGroupChanged(), genDT(2010, 8, 8, 17, 24, 19));
    QVERIFY(m_db->metadata()->lastSelectedGroup() != 0);
    QCOMPARE(m_db->metadata()->lastSelectedGroup()->name(), QString("NewDatabase"));
    QVERIFY(m_db->metadata()->lastTopVisibleGroup() == m_db->metadata()->lastSelectedGroup());
}

void TestParser::testGroups()
{
    QVERIFY(m_db->rootGroup() != 0);
    QCOMPARE(m_db->rootGroup()->name(), QString("NewDatabase"));
    QCOMPARE(m_db->rootGroup()->uuid().toBase64(), QString("zKuE27EWr0mlU75b2SRkTQ=="));
    QVERIFY(m_db->rootGroup()->isExpanded());
    TimeInfo ti = m_db->rootGroup()->timeInfo();
    QCOMPARE(ti.lastModificationTime(), genDT(2010, 8, 8, 17, 24, 27));
    QCOMPARE(ti.creationTime(), genDT(2010, 8, 7, 17, 24, 27));
    QCOMPARE(ti.lastAccessTime(), genDT(2010, 8, 9, 9, 9, 44));
    QCOMPARE(ti.expiryTime(), genDT(2010, 8, 8, 17, 24, 17));
    QVERIFY(!ti.expires());
    QCOMPARE(ti.usageCount(), 2);
    QCOMPARE(ti.locationChanged(), genDT(2010, 8, 8, 17, 24, 27));

    QCOMPARE(m_db->rootGroup()->children().size(), 3);
    QCOMPARE(m_db->rootGroup()->children().at(0)->uuid().toBase64(), QString("abLbFtNUfEi5TmbaxiW6yg=="));
    QCOMPARE(m_db->rootGroup()->children().at(1)->uuid().toBase64(), QString("u1lTRAICOkWv5QSl2xyU8w=="));
    QCOMPARE(m_db->rootGroup()->children().at(2)->uuid().toBase64(), QString("7PAwxNhPaE2klutz45i2xg=="));
    QVERIFY(m_db->metadata()->recycleBin() == m_db->rootGroup()->children().at(2));
}

QTEST_MAIN(TestParser);

#include "TestParser.moc"
