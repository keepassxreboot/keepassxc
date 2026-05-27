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

#include "TestNetrcExporter.h"

#include <QBuffer>
#include <QTest>

#include "core/Group.h"
#include "core/Tools.h"
#include "core/Totp.h"
#include "crypto/Crypto.h"
#include "format/NetrcExporter.h"

QTEST_GUILESS_MAIN(TestNetrcExporter)

void TestNetrcExporter::init()
{
    m_db = QSharedPointer<Database>::create();
    m_netrcExporter = QSharedPointer<NetrcExporter>::create();
}

void TestNetrcExporter::initTestCase()
{
    Crypto::init();
}

void TestNetrcExporter::cleanup()
{
}

void TestNetrcExporter::testExport()
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
    entry->setTotp(Totp::createSettings("DFDF", Totp::DEFAULT_DIGITS, Totp::DEFAULT_STEP));
    entry->setIcon(5);

    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    m_netrcExporter->exportDatabase(&buffer, m_db);
    auto exported = QString::fromUtf8(buffer.buffer());

    QString expectedResult = QString()
                                 .append("machine\tTest Entry Title\tlogin\tTest Username\tpassword\tTest Password\t\n"
                                         "machine\thttp://test.url\tlogin\tTest Username\tpassword\tTest Password\t\n");

    QVERIFY(exported.startsWith(expectedResult));
}

void TestNetrcExporter::testEmptyDatabase()
{
    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    m_netrcExporter->exportDatabase(&buffer, m_db);

    QCOMPARE(QString::fromUtf8(buffer.buffer().constData()), QString(""));
}

void TestNetrcExporter::testNestedGroups()
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
    m_netrcExporter->exportDatabase(&buffer, m_db);
    auto exported = QString::fromUtf8(buffer.buffer());
    QVERIFY(exported.startsWith(
        QString()
            .append("")));
}
