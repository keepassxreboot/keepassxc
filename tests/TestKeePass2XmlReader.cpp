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
#include "format/KeePass2XmlReader.h"
#include "config-keepassx-tests.h"

namespace QTest {
    template<>
    char *toString(const Uuid &uuid)
    {
        QByteArray ba = "Uuid(";
        ba += uuid.toBase64();
        ba += ")";
        return qstrdup(ba.data());
    }
}

class TestParser : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testMetadata();
    void testCustomIcons();
    void testCustomData();
    void testGroupRoot();
    void testGroup1();
    void testGroup2();
    void testEntry1();
    void testEntry2();
    void testEntryHistory();
    void testDeletedObjects();

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
    KeePass2XmlReader* reader = new KeePass2XmlReader();
    QString xmlFile = QString(KEEPASSX_TEST_DIR).append("/NewDatabase.xml");
    m_db = reader->readDatabase(xmlFile);
    QVERIFY(!reader->error());
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
    QCOMPARE(m_db->metadata()->autoEnableVisualHiding(), false);
    QCOMPARE(m_db->metadata()->recycleBinEnabled(), true);
    QVERIFY(m_db->metadata()->recycleBin() != 0);
    QCOMPARE(m_db->metadata()->recycleBin()->name(), QString("Recycle Bin"));
    QCOMPARE(m_db->metadata()->recycleBinChanged(), genDT(2010, 8, 25, 16, 12, 57));
    QVERIFY(m_db->metadata()->entryTemplatesGroup() == 0);
    QCOMPARE(m_db->metadata()->entryTemplatesGroupChanged(), genDT(2010, 8, 8, 17, 24, 19));
    QVERIFY(m_db->metadata()->lastSelectedGroup() != 0);
    QCOMPARE(m_db->metadata()->lastSelectedGroup()->name(), QString("NewDatabase"));
    QVERIFY(m_db->metadata()->lastTopVisibleGroup() == m_db->metadata()->lastSelectedGroup());
}

void TestParser::testCustomIcons()
{
    QCOMPARE(m_db->metadata()->customIcons().size(), 1);
    Uuid uuid = Uuid::fromBase64("++vyI+daLk6omox4a6kQGA==");
    QVERIFY(m_db->metadata()->customIcons().contains(uuid));
    QIcon icon = m_db->metadata()->customIcon(uuid);
    QImage img = icon.pixmap(16, 16).toImage();
    for (int x=0; x<16; x++) {
        for (int y=0; y<16; y++) {
            QRgb rgb = img.pixel(x, y);
            QCOMPARE(qRed(rgb), 128);
            QCOMPARE(qGreen(rgb), 0);
            QCOMPARE(qBlue(rgb), 128);
        }
    }
}

void TestParser::testCustomData()
{
    QHash<QString, QString> customFields = m_db->metadata()->customFields();

    QCOMPARE(customFields.size(), 2);
    QCOMPARE(customFields.value("A Sample Test Key"), QString("valu"));
    QCOMPARE(customFields.value("custom key"), QString("blub"));
}

void TestParser::testGroupRoot()
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
    QCOMPARE(group->autoTypeEnabled(), -1);
    QCOMPARE(group->searchingEnabed(), -1);
    QCOMPARE(group->lastTopVisibleEntry()->uuid().toBase64(), QString("+wSUOv6qf0OzW8/ZHAs2sA=="));

    QCOMPARE(group->children().size(), 3);
    QVERIFY(m_db->metadata()->recycleBin() == m_db->rootGroup()->children().at(2));

    QCOMPARE(group->entries().size(), 2);
}

void TestParser::testGroup1()
{
    const Group* group = m_db->rootGroup()->children().at(0);

    QCOMPARE(group->uuid().toBase64(), QString("AaUYVdXsI02h4T1RiAlgtg=="));
    QCOMPARE(group->name(), QString("General"));
    QCOMPARE(group->notes(), QString("Group Notez"));
    QCOMPARE(group->iconNumber(), 48);
    QCOMPARE(group->iconUuid(), Uuid());
    QCOMPARE(group->isExpanded(), true);
    QCOMPARE(group->defaultAutoTypeSequence(), QString("{Password}{ENTER}"));
    QCOMPARE(group->autoTypeEnabled(), 1);
    QCOMPARE(group->searchingEnabed(), 0);
    QVERIFY(!group->lastTopVisibleEntry());
}

void TestParser::testGroup2()
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

void TestParser::testEntry1()
{
    const Entry* entry = m_db->rootGroup()->entries().at(0);

    QCOMPARE(entry->uuid().toBase64(), QString("+wSUOv6qf0OzW8/ZHAs2sA=="));
    QCOMPARE(entry->iconNumber(), 0);
    QCOMPARE(entry->iconUuid(), Uuid());
    QVERIFY(!entry->foregroundColor().isValid());
    QVERIFY(!entry->backgroundColor().isValid());
    QCOMPARE(entry->overrideUrl(), QString(""));

    const TimeInfo ti = entry->timeInfo();
    QCOMPARE(ti.lastModificationTime(), genDT(2010, 8, 25, 16, 19, 25));
    QCOMPARE(ti.creationTime(), genDT(2010, 8, 25, 16, 13, 54));
    QCOMPARE(ti.lastAccessTime(), genDT(2010, 8, 25, 16, 19, 25));
    QCOMPARE(ti.expiryTime(), genDT(2010, 8, 25, 16, 12, 57));
    QVERIFY(!ti.expires());
    QCOMPARE(ti.usageCount(), 8);
    QCOMPARE(ti.locationChanged(), genDT(2010, 8, 25, 16, 13, 54));

    QHash<QString,QString> attrs = entry->attributes();
    QCOMPARE(attrs.take("Notes"), QString("Notes"));
    QCOMPARE(attrs.take("Password"), QString("Password"));
    QCOMPARE(attrs.take("Title"), QString("Sample Entry 1"));
    QCOMPARE(attrs.take("URL"), QString(""));
    QCOMPARE(attrs.take("UserName"), QString("User Name"));
    QVERIFY(attrs.isEmpty());

    QCOMPARE(entry->title(), entry->attributes().value("Title"));
    QCOMPARE(entry->url(), entry->attributes().value("URL"));
    QCOMPARE(entry->username(), entry->attributes().value("UserName"));
    QCOMPARE(entry->password(), entry->attributes().value("Password"));
    QCOMPARE(entry->notes(), entry->attributes().value("Notes"));

    QCOMPARE(entry->attachments().size(), 0);
    QCOMPARE(entry->autoTypeEnabled(), false);
    QCOMPARE(entry->autoTypeObfuscation(), 0);
    QCOMPARE(entry->defaultAutoTypeSequence(), QString(""));
    QCOMPARE(entry->autoTypeAssociations().size(), 1);
    const AutoTypeAssociation assoc1 = entry->autoTypeAssociations().at(0);
    QCOMPARE(assoc1.window, QString("Target Window"));
    QCOMPARE(assoc1.sequence, QString(""));
}

void TestParser::testEntry2()
{
    const Entry* entry = m_db->rootGroup()->entries().at(1);

    QCOMPARE(entry->uuid().toBase64(), QString("4jbADG37hkiLh2O0qUdaOQ=="));
    QCOMPARE(entry->iconNumber(), 0);
    QCOMPARE(entry->iconUuid().toBase64(), QString("++vyI+daLk6omox4a6kQGA=="));
    // TODO test entry->icon()
    QCOMPARE(entry->foregroundColor(), QColor(255, 0, 0));
    QCOMPARE(entry->backgroundColor(), QColor(255, 255, 0));
    QCOMPARE(entry->overrideUrl(), QString("http://override.net/"));

    const TimeInfo ti = entry->timeInfo();
    QCOMPARE(ti.usageCount(), 7);

    QHash<QString,QString> attrs = entry->attributes();
    QCOMPARE(attrs.take("CustomString"), QString("isavalue"));
    QCOMPARE(attrs.take("Notes"), QString(""));
    QCOMPARE(attrs.take("Password"), QString("Jer60Hz8o9XHvxBGcRqT"));
    QCOMPARE(attrs.take("Protected String"), QString("y")); // TODO should have a protection attribute
    QCOMPARE(attrs.take("Title"), QString("Sample Entry 2"));
    QCOMPARE(attrs.take("URL"), QString("http://www.keepassx.org/"));
    QCOMPARE(attrs.take("UserName"), QString("notDEFUSERNAME"));
    QVERIFY(attrs.isEmpty());

    QCOMPARE(entry->attachments().size(), 1);
    QCOMPARE(QString(entry->attachments().value("testattach.txt")), QString("42"));

    QCOMPARE(entry->autoTypeEnabled(), true);
    QCOMPARE(entry->autoTypeObfuscation(), 1);
    QCOMPARE(entry->defaultAutoTypeSequence(), QString("{USERNAME}{TAB}{PASSWORD}{ENTER}"));
    QCOMPARE(entry->autoTypeAssociations().size(), 2);
    const AutoTypeAssociation assoc1 = entry->autoTypeAssociations().at(0);
    QCOMPARE(assoc1.window, QString("Target Window"));
    QCOMPARE(assoc1.sequence, QString("{Title}{UserName}"));
    const AutoTypeAssociation assoc2 = entry->autoTypeAssociations().at(1);
    QCOMPARE(assoc2.window, QString("Target Window 2"));
    QCOMPARE(assoc2.sequence, QString("{Title}{UserName} test"));
}

void TestParser::testEntryHistory()
{
    const Entry* entryMain = m_db->rootGroup()->entries().first();
    QCOMPARE(entryMain->historyItems().size(), 2);

    {
        const Entry* entry = entryMain->historyItems().at(0);
        QCOMPARE(entry->uuid(), entryMain->uuid());
        QVERIFY(!entry->parent());
        QCOMPARE(entry->children().size(), 0);
        QCOMPARE(entry->timeInfo().lastModificationTime(), genDT(2010, 8, 25, 16, 13, 54));
        QCOMPARE(entry->timeInfo().usageCount(), 3);
        QCOMPARE(entry->title(), QString("Sample Entry"));
        QCOMPARE(entry->url(), QString("http://www.somesite.com/"));
    }

    {
        const Entry* entry = entryMain->historyItems().at(1);
        QCOMPARE(entry->uuid(), entryMain->uuid());
        QVERIFY(!entry->parent());
        QCOMPARE(entry->children().size(), 0);
        QCOMPARE(entry->timeInfo().lastModificationTime(), genDT(2010, 8, 25, 16, 15, 43));
        QCOMPARE(entry->timeInfo().usageCount(), 7);
        QCOMPARE(entry->title(), QString("Sample Entry 1"));
        QCOMPARE(entry->url(), QString("http://www.somesite.com/"));
    }
}

void TestParser::testDeletedObjects()
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

QTEST_MAIN(TestParser);

#include "TestParser.moc"
