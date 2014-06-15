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

#include "TestKeePass2XmlReader.h"

#include <QFile>
#include <QTest>

#include "tests.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "format/KeePass2XmlReader.h"
#include "config-keepassx-tests.h"

QTEST_GUILESS_MAIN(TestKeePass2XmlReader)

namespace QTest {
    template<>
    char* toString(const Uuid& uuid)
    {
        QByteArray ba = "Uuid(";
        ba += uuid.toBase64().toLatin1().constData();
        ba += ")";
        return qstrdup(ba.constData());
    }

    template<>
    char* toString(const Group::TriState& triState)
    {
        QString value;

        if (triState == Group::Inherit) {
            value = "null";
        }
        else if (triState == Group::Enable) {
            value = "true";
        }
        else {
            value = "false";
        }

        return qstrdup(value.toLocal8Bit().constData());
    }
}

QDateTime TestKeePass2XmlReader::genDT(int year, int month, int day, int hour, int min, int second)
{
    QDate date(year, month, day);
    QTime time(hour, min, second);
    return QDateTime(date, time, Qt::UTC);
}

void TestKeePass2XmlReader::initTestCase()
{
    QVERIFY(Crypto::init());

    KeePass2XmlReader reader;
    QString xmlFile = QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.xml");
    m_db = reader.readDatabase(xmlFile);
    QVERIFY(m_db);
    QVERIFY(!reader.hasError());
}

void TestKeePass2XmlReader::testMetadata()
{
    QCOMPARE(m_db->metadata()->generator(), QString("KeePass"));
    QCOMPARE(m_db->metadata()->name(), QString("ANAME"));
    QCOMPARE(m_db->metadata()->nameChanged(), genDT(2010, 8, 8, 17, 24, 53));
    QCOMPARE(m_db->metadata()->description(), QString("ADESC"));
    QCOMPARE(m_db->metadata()->descriptionChanged(), genDT(2010, 8, 8, 17, 27, 12));
    QCOMPARE(m_db->metadata()->defaultUserName(), QString("DEFUSERNAME"));
    QCOMPARE(m_db->metadata()->defaultUserNameChanged(), genDT(2010, 8, 8, 17, 27, 45));
    QCOMPARE(m_db->metadata()->maintenanceHistoryDays(), 127);
    QCOMPARE(m_db->metadata()->color(), QColor(0xff, 0xef, 0x00));
    QCOMPARE(m_db->metadata()->masterKeyChanged(), genDT(2012, 4, 5, 17, 9, 34));
    QCOMPARE(m_db->metadata()->masterKeyChangeRec(), 101);
    QCOMPARE(m_db->metadata()->masterKeyChangeForce(), -1);
    QCOMPARE(m_db->metadata()->protectTitle(), false);
    QCOMPARE(m_db->metadata()->protectUsername(), true);
    QCOMPARE(m_db->metadata()->protectPassword(), false);
    QCOMPARE(m_db->metadata()->protectUrl(), true);
    QCOMPARE(m_db->metadata()->protectNotes(), false);
    QCOMPARE(m_db->metadata()->recycleBinEnabled(), true);
    QVERIFY(m_db->metadata()->recycleBin() != Q_NULLPTR);
    QCOMPARE(m_db->metadata()->recycleBin()->name(), QString("Recycle Bin"));
    QCOMPARE(m_db->metadata()->recycleBinChanged(), genDT(2010, 8, 25, 16, 12, 57));
    QVERIFY(m_db->metadata()->entryTemplatesGroup() == Q_NULLPTR);
    QCOMPARE(m_db->metadata()->entryTemplatesGroupChanged(), genDT(2010, 8, 8, 17, 24, 19));
    QVERIFY(m_db->metadata()->lastSelectedGroup() != Q_NULLPTR);
    QCOMPARE(m_db->metadata()->lastSelectedGroup()->name(), QString("NewDatabase"));
    QVERIFY(m_db->metadata()->lastTopVisibleGroup() == m_db->metadata()->lastSelectedGroup());
    QCOMPARE(m_db->metadata()->historyMaxItems(), -1);
    QCOMPARE(m_db->metadata()->historyMaxSize(), 5242880);
}

void TestKeePass2XmlReader::testCustomIcons()
{
    QCOMPARE(m_db->metadata()->customIcons().size(), 1);
    Uuid uuid = Uuid::fromBase64("++vyI+daLk6omox4a6kQGA==");
    QVERIFY(m_db->metadata()->customIcons().contains(uuid));
    QImage icon = m_db->metadata()->customIcon(uuid);
    QCOMPARE(icon.width(), 16);
    QCOMPARE(icon.height(), 16);

    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            QRgb rgb = icon.pixel(x, y);
            QCOMPARE(qRed(rgb), 128);
            QCOMPARE(qGreen(rgb), 0);
            QCOMPARE(qBlue(rgb), 128);
        }
    }
}

void TestKeePass2XmlReader::testCustomData()
{
    QHash<QString, QString> customFields = m_db->metadata()->customFields();

    QCOMPARE(customFields.size(), 2);
    QCOMPARE(customFields.value("A Sample Test Key"), QString("valu"));
    QCOMPARE(customFields.value("custom key"), QString("blub"));
}

void TestKeePass2XmlReader::testGroupRoot()
{
    const Group* group = m_db->rootGroup();
    QVERIFY(group);
    QCOMPARE(group->uuid().toBase64(), QString("lmU+9n0aeESKZvcEze+bRg=="));
    QCOMPARE(group->name(), QString("NewDatabase"));
    QCOMPARE(group->notes(), QString(""));
    QCOMPARE(group->iconNumber(), 49);
    QCOMPARE(group->iconUuid(), Uuid());
    QVERIFY(group->isExpanded());
    TimeInfo ti = group->timeInfo();
    QCOMPARE(ti.lastModificationTime(), genDT(2010, 8, 8, 17, 24, 27));
    QCOMPARE(ti.creationTime(), genDT(2010, 8, 7, 17, 24, 27));
    QCOMPARE(ti.lastAccessTime(), genDT(2010, 8, 9, 9, 9, 44));
    QCOMPARE(ti.expiryTime(), genDT(2010, 8, 8, 17, 24, 17));
    QVERIFY(!ti.expires());
    QCOMPARE(ti.usageCount(), 52);
    QCOMPARE(ti.locationChanged(), genDT(2010, 8, 8, 17, 24, 27));
    QCOMPARE(group->defaultAutoTypeSequence(), QString(""));
    QCOMPARE(group->autoTypeEnabled(), Group::Inherit);
    QCOMPARE(group->searchingEnabled(), Group::Inherit);
    QCOMPARE(group->lastTopVisibleEntry()->uuid().toBase64(), QString("+wSUOv6qf0OzW8/ZHAs2sA=="));

    QCOMPARE(group->children().size(), 3);
    QVERIFY(m_db->metadata()->recycleBin() == m_db->rootGroup()->children().at(2));

    QCOMPARE(group->entries().size(), 2);
}

void TestKeePass2XmlReader::testGroup1()
{
    const Group* group = m_db->rootGroup()->children().at(0);

    QCOMPARE(group->uuid().toBase64(), QString("AaUYVdXsI02h4T1RiAlgtg=="));
    QCOMPARE(group->name(), QString("General"));
    QCOMPARE(group->notes(), QString("Group Notez"));
    QCOMPARE(group->iconNumber(), 48);
    QCOMPARE(group->iconUuid(), Uuid());
    QCOMPARE(group->isExpanded(), true);
    QCOMPARE(group->defaultAutoTypeSequence(), QString("{Password}{ENTER}"));
    QCOMPARE(group->autoTypeEnabled(), Group::Enable);
    QCOMPARE(group->searchingEnabled(), Group::Disable);
    QVERIFY(!group->lastTopVisibleEntry());
}

void TestKeePass2XmlReader::testGroup2()
{
    const Group* group = m_db->rootGroup()->children().at(1);

    QCOMPARE(group->uuid().toBase64(), QString("1h4NtL5DK0yVyvaEnN//4A=="));
    QCOMPARE(group->name(), QString("Windows"));
    QCOMPARE(group->isExpanded(), false);

    QCOMPARE(group->children().size(), 1);
    const Group* child = group->children().first();

    QCOMPARE(child->uuid().toBase64(), QString("HoYE/BjLfUSW257pCHJ/eA=="));
    QCOMPARE(child->name(), QString("Subsub"));
    QCOMPARE(child->entries().size(), 1);

    const Entry* entry = child->entries().first();
    QCOMPARE(entry->uuid().toBase64(), QString("GZpdQvGXOU2kaKRL/IVAGg=="));
    QCOMPARE(entry->title(), QString("Subsub Entry"));
}

void TestKeePass2XmlReader::testEntry1()
{
    const Entry* entry = m_db->rootGroup()->entries().at(0);

    QCOMPARE(entry->uuid().toBase64(), QString("+wSUOv6qf0OzW8/ZHAs2sA=="));
    QCOMPARE(entry->historyItems().size(), 2);
    QCOMPARE(entry->iconNumber(), 0);
    QCOMPARE(entry->iconUuid(), Uuid());
    QVERIFY(!entry->foregroundColor().isValid());
    QVERIFY(!entry->backgroundColor().isValid());
    QCOMPARE(entry->overrideUrl(), QString(""));
    QCOMPARE(entry->tags(), QString("a b c"));

    const TimeInfo ti = entry->timeInfo();
    QCOMPARE(ti.lastModificationTime(), genDT(2010, 8, 25, 16, 19, 25));
    QCOMPARE(ti.creationTime(), genDT(2010, 8, 25, 16, 13, 54));
    QCOMPARE(ti.lastAccessTime(), genDT(2010, 8, 25, 16, 19, 25));
    QCOMPARE(ti.expiryTime(), genDT(2010, 8, 25, 16, 12, 57));
    QVERIFY(!ti.expires());
    QCOMPARE(ti.usageCount(), 8);
    QCOMPARE(ti.locationChanged(), genDT(2010, 8, 25, 16, 13, 54));

    QList<QString> attrs = entry->attributes()->keys();
    QCOMPARE(entry->attributes()->value("Notes"), QString("Notes"));
    QVERIFY(!entry->attributes()->isProtected("Notes"));
    QVERIFY(attrs.removeOne("Notes"));
    QCOMPARE(entry->attributes()->value("Password"), QString("Password"));
    QVERIFY(!entry->attributes()->isProtected("Password"));
    QVERIFY(attrs.removeOne("Password"));
    QCOMPARE(entry->attributes()->value("Title"), QString("Sample Entry 1"));
    QVERIFY(!entry->attributes()->isProtected("Title"));
    QVERIFY(attrs.removeOne("Title"));
    QCOMPARE(entry->attributes()->value("URL"), QString(""));
    QVERIFY(entry->attributes()->isProtected("URL"));
    QVERIFY(attrs.removeOne("URL"));
    QCOMPARE(entry->attributes()->value("UserName"), QString("User Name"));
    QVERIFY(entry->attributes()->isProtected("UserName"));
    QVERIFY(attrs.removeOne("UserName"));
    QVERIFY(attrs.isEmpty());

    QCOMPARE(entry->title(), entry->attributes()->value("Title"));
    QCOMPARE(entry->url(), entry->attributes()->value("URL"));
    QCOMPARE(entry->username(), entry->attributes()->value("UserName"));
    QCOMPARE(entry->password(), entry->attributes()->value("Password"));
    QCOMPARE(entry->notes(), entry->attributes()->value("Notes"));

    QCOMPARE(entry->attachments()->keys().size(), 1);
    QCOMPARE(entry->attachments()->value("myattach.txt"), QByteArray("abcdefghijk"));
    QCOMPARE(entry->historyItems().at(0)->attachments()->keys().size(), 1);
    QCOMPARE(entry->historyItems().at(0)->attachments()->value("myattach.txt"), QByteArray("0123456789"));
    QCOMPARE(entry->historyItems().at(1)->attachments()->keys().size(), 1);
    QCOMPARE(entry->historyItems().at(1)->attachments()->value("myattach.txt"), QByteArray("abcdefghijk"));

    QCOMPARE(entry->autoTypeEnabled(), false);
    QCOMPARE(entry->autoTypeObfuscation(), 0);
    QCOMPARE(entry->defaultAutoTypeSequence(), QString(""));
    QCOMPARE(entry->autoTypeAssociations()->size(), 1);
    const AutoTypeAssociations::Association assoc1 = entry->autoTypeAssociations()->get(0);
    QCOMPARE(assoc1.window, QString("Target Window"));
    QCOMPARE(assoc1.sequence, QString(""));
}

void TestKeePass2XmlReader::testEntry2()
{
    const Entry* entry = m_db->rootGroup()->entries().at(1);

    QCOMPARE(entry->uuid().toBase64(), QString("4jbADG37hkiLh2O0qUdaOQ=="));
    QCOMPARE(entry->iconNumber(), 0);
    QCOMPARE(entry->iconUuid().toBase64(), QString("++vyI+daLk6omox4a6kQGA=="));
    // TODO: test entry->icon()
    QCOMPARE(entry->foregroundColor(), QColor(255, 0, 0));
    QCOMPARE(entry->backgroundColor(), QColor(255, 255, 0));
    QCOMPARE(entry->overrideUrl(), QString("http://override.net/"));
    QCOMPARE(entry->tags(), QString(""));

    const TimeInfo ti = entry->timeInfo();
    QCOMPARE(ti.usageCount(), 7);

    QList<QString> attrs = entry->attributes()->keys();
    QCOMPARE(entry->attributes()->value("CustomString"), QString("isavalue"));
    QVERIFY(attrs.removeOne("CustomString"));
    QCOMPARE(entry->attributes()->value("Notes"), QString(""));
    QVERIFY(attrs.removeOne("Notes"));
    QCOMPARE(entry->attributes()->value("Password"), QString("Jer60Hz8o9XHvxBGcRqT"));
    QVERIFY(attrs.removeOne("Password"));
    QCOMPARE(entry->attributes()->value("Protected String"), QString("y")); // TODO: should have a protection attribute
    QVERIFY(attrs.removeOne("Protected String"));
    QCOMPARE(entry->attributes()->value("Title"), QString("Sample Entry 2"));
    QVERIFY(attrs.removeOne("Title"));
    QCOMPARE(entry->attributes()->value("URL"), QString("http://www.keepassx.org/"));
    QVERIFY(attrs.removeOne("URL"));
    QCOMPARE(entry->attributes()->value("UserName"), QString("notDEFUSERNAME"));
    QVERIFY(attrs.removeOne("UserName"));
    QVERIFY(attrs.isEmpty());

    QCOMPARE(entry->attachments()->keys().size(), 1);
    QCOMPARE(QString::fromLatin1(entry->attachments()->value("myattach.txt")), QString("abcdefghijk"));

    QCOMPARE(entry->autoTypeEnabled(), true);
    QCOMPARE(entry->autoTypeObfuscation(), 1);
    QCOMPARE(entry->defaultAutoTypeSequence(), QString("{USERNAME}{TAB}{PASSWORD}{ENTER}"));
    QCOMPARE(entry->autoTypeAssociations()->size(), 2);
    const AutoTypeAssociations::Association assoc1 = entry->autoTypeAssociations()->get(0);
    QCOMPARE(assoc1.window, QString("Target Window"));
    QCOMPARE(assoc1.sequence, QString("{Title}{UserName}"));
    const AutoTypeAssociations::Association assoc2 = entry->autoTypeAssociations()->get(1);
    QCOMPARE(assoc2.window, QString("Target Window 2"));
    QCOMPARE(assoc2.sequence, QString("{Title}{UserName} test"));
}

void TestKeePass2XmlReader::testEntryHistory()
{
    const Entry* entryMain = m_db->rootGroup()->entries().first();
    QCOMPARE(entryMain->historyItems().size(), 2);

    {
        const Entry* entry = entryMain->historyItems().at(0);
        QCOMPARE(entry->uuid(), entryMain->uuid());
        QVERIFY(!entry->parent());
        QCOMPARE(entry->timeInfo().lastModificationTime(), genDT(2010, 8, 25, 16, 13, 54));
        QCOMPARE(entry->timeInfo().usageCount(), 3);
        QCOMPARE(entry->title(), QString("Sample Entry"));
        QCOMPARE(entry->url(), QString("http://www.somesite.com/"));
    }

    {
        const Entry* entry = entryMain->historyItems().at(1);
        QCOMPARE(entry->uuid(), entryMain->uuid());
        QVERIFY(!entry->parent());
        QCOMPARE(entry->timeInfo().lastModificationTime(), genDT(2010, 8, 25, 16, 15, 43));
        QCOMPARE(entry->timeInfo().usageCount(), 7);
        QCOMPARE(entry->title(), QString("Sample Entry 1"));
        QCOMPARE(entry->url(), QString("http://www.somesite.com/"));
    }
}

void TestKeePass2XmlReader::testDeletedObjects()
{
    QList<DeletedObject> objList = m_db->deletedObjects();
    DeletedObject delObj;

    delObj = objList.takeFirst();
    QCOMPARE(delObj.uuid.toBase64(), QString("5K/bzWCSmkCv5OZxYl4N/w=="));
    QCOMPARE(delObj.deletionTime, genDT(2010, 8, 25, 16, 14, 12));

    delObj = objList.takeFirst();
    QCOMPARE(delObj.uuid.toBase64(), QString("80h8uSNWgkKhKCp1TgXF7g=="));
    QCOMPARE(delObj.deletionTime, genDT(2010, 8, 25, 16, 14, 14));

    QVERIFY(objList.isEmpty());
}

void TestKeePass2XmlReader::testBroken()
{
    QFETCH(QString, baseName);

    KeePass2XmlReader reader;
    QString xmlFile = QString("%1/%2.xml").arg(KEEPASSX_TEST_DATA_DIR, baseName);
    QVERIFY(QFile::exists(xmlFile));
    QScopedPointer<Database> db(reader.readDatabase(xmlFile));
    QVERIFY(reader.hasError());
}

void TestKeePass2XmlReader::testBroken_data()
{
    QTest::addColumn<QString>("baseName");

    QTest::newRow("BrokenNoGroupUuid") << "BrokenNoGroupUuid";
    QTest::newRow("BrokenNoEntryUuid") << "BrokenNoEntryUuid";
    QTest::newRow("BrokenNoRootGroup") << "BrokenNoRootGroup";
    QTest::newRow("BrokenTwoRoots") << "BrokenTwoRoots";
    QTest::newRow("BrokenTwoRootGroups") << "BrokenTwoRootGroups";
}

void TestKeePass2XmlReader::cleanupTestCase()
{
    delete m_db;
}
