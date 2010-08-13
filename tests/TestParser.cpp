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
    QCOMPARE(m_db->metadata()->generator(), QLatin1String("KeePass"));
    QCOMPARE(m_db->metadata()->name(), QLatin1String("ANAME"));
    QCOMPARE(m_db->metadata()->nameChanged(), genDT(2010, 8, 8, 17, 24, 53));
    QCOMPARE(m_db->metadata()->description(), QLatin1String("ADESC"));
    QCOMPARE(m_db->metadata()->descriptionChanged(), genDT(2010, 8, 8, 17, 27, 12));
    QCOMPARE(m_db->metadata()->defaultUserName(), QLatin1String("DEFUSERNAME"));
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
    QCOMPARE(m_db->metadata()->recycleBin()->name(), QLatin1String("Recycle Bin"));
    QCOMPARE(m_db->metadata()->recycleBinChanged(), genDT(2010, 8, 8, 17, 24, 17));
    QVERIFY(m_db->metadata()->entryTemplatesGroup() == 0);
    QCOMPARE(m_db->metadata()->entryTemplatesGroupChanged(), genDT(2010, 8, 8, 17, 24, 19));
    QVERIFY(m_db->metadata()->lastSelectedGroup() != 0);
    QCOMPARE(m_db->metadata()->lastSelectedGroup()->name(), QLatin1String("NewDatabase"));
    QVERIFY(m_db->metadata()->lastTopVisibleGroup() != 0);
    QCOMPARE(m_db->metadata()->lastTopVisibleGroup()->name(), QLatin1String("NewDatabase"));
}

void TestParser::testGroups()
{
    QVERIFY(m_db->rootGroup()->name() == QLatin1String("NewDatabase"));
    QVERIFY(m_db->rootGroup()->uuid().toBase64() == QLatin1String("zKuE27EWr0mlU75b2SRkTQ=="));
    QVERIFY(m_db->rootGroup()->isExpanded() == true);
}

QTEST_MAIN(TestParser);

#include "TestParser.moc"
