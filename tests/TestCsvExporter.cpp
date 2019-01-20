/*
 *  Copyright (C) 2015 Florian Geyer <blueice@fobos.de>
 *  Copyright (C) 2015 Felix Geyer <debfx@fobos.de>
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

#include "TestCsvExporter.h"
#include "TestGlobal.h"

#include <QBuffer>

#include "crypto/Crypto.h"
#include "format/CsvExporter.h"

QTEST_GUILESS_MAIN(TestCsvExporter)

const QString TestCsvExporter::ExpectedHeaderLine =
    QString("\"Group\",\"Title\",\"Username\",\"Password\",\"URL\",\"Notes\"\n");

void TestCsvExporter::init()
{
    m_db = QSharedPointer<Database>::create();
    m_csvExporter = QSharedPointer<CsvExporter>::create();
}

void TestCsvExporter::initTestCase()
{
    Crypto::init();
}

void TestCsvExporter::cleanup()
{
}

void TestCsvExporter::testExport()
{
    Group* groupRoot = m_db->rootGroup();
    auto* group = new Group();
    group->setName("Test Group Name");
    group->setParent(groupRoot);
    auto* entry = new Entry();
    entry->setGroup(group);
    entry->setTitle("Test Entry Title");
    entry->setUsername("Test Username");
    entry->setPassword("Test Password");
    entry->setUrl("http://test.url");
    entry->setNotes("Test Notes");

    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    m_csvExporter->exportDatabase(&buffer, m_db);

    QString expectedResult = QString()
                                 .append(ExpectedHeaderLine)
                                 .append("\"Root/Test Group Name\",\"Test Entry Title\",\"Test Username\",\"Test "
                                         "Password\",\"http://test.url\",\"Test Notes\"\n");

    QCOMPARE(QString::fromUtf8(buffer.buffer().constData()), expectedResult);
}

void TestCsvExporter::testEmptyDatabase()
{
    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    m_csvExporter->exportDatabase(&buffer, m_db);

    QCOMPARE(QString::fromUtf8(buffer.buffer().constData()), ExpectedHeaderLine);
}

void TestCsvExporter::testNestedGroups()
{
    Group* groupRoot = m_db->rootGroup();
    auto* group = new Group();
    group->setName("Test Group Name");
    group->setParent(groupRoot);
    auto* childGroup = new Group();
    childGroup->setName("Test Sub Group Name");
    childGroup->setParent(group);
    auto* entry = new Entry();
    entry->setGroup(childGroup);
    entry->setTitle("Test Entry Title");

    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    m_csvExporter->exportDatabase(&buffer, m_db);

    QCOMPARE(QString::fromUtf8(buffer.buffer().constData()),
             QString()
                 .append(ExpectedHeaderLine)
                 .append("\"Root/Test Group Name/Test Sub Group Name\",\"Test Entry Title\",\"\",\"\",\"\",\"\"\n"));
}
