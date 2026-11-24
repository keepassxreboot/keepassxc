/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "TestXmlImport.h"

#include <memory>

#include <QFile>
#include <QIODevice>
#include <QRandomGenerator>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>

#include "core/AutoTypeAssociations.h"
#include "core/Database.h"
#include "core/DatabaseSettingsDiff.h"
#include "core/Entry.h"
#include "core/EntryAttachments.h"
#include "core/EntryAttributes.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"

QTEST_GUILESS_MAIN(TestXmlImport)

namespace
{
    QByteArray binaryPayload(int size)
    {
        // Contains NUL bytes, 0xFF and every other byte value, so any text-mode handling shows up
        QByteArray data;
        for (int i = 0; i < size; ++i) {
            data.append(static_cast<char>(i * 31 + 7));
        }
        return data;
    }

    QString uuidXml(const QUuid& uuid)
    {
        return QString::fromLatin1(uuid.toRfc4122().toBase64());
    }

    bool writeFile(const QString& path, const QByteArray& data)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        return file.write(data) == data.size();
    }

    bool exportXml(Database* db, const QString& path)
    {
        QByteArray xml;
        QString error;
        return db->extract(xml, &error) && writeFile(path, xml);
    }

    // A write-only device that discards what is written and fails after some number of bytes (a full disk, a closed
    // pipe).
    class FailingDevice : public QIODevice
    {
    public:
        explicit FailingDevice(qint64 failAfterBytes)
            : m_failAfter(failAfterBytes)
        {
            open(QIODevice::WriteOnly);
        }

        bool isSequential() const override
        {
            return true;
        }

    protected:
        qint64 readData(char*, qint64) override
        {
            return -1;
        }

        qint64 writeData(const char*, qint64 len) override
        {
            if (m_total + len > m_failAfter) {
                setErrorString("disk full (simulated)");
                return -1;
            }
            m_total += len;
            return len;
        }

    private:
        qint64 m_failAfter;
        qint64 m_total = 0;
    };

    // Bytes that do not compress, so the export is large with and without gzip
    QByteArray incompressiblePayload(int size)
    {
        QRandomGenerator generator(1234);
        QByteArray data(size, 0);
        for (int i = 0; i < size; ++i) {
            data[i] = static_cast<char>(generator.generate() & 0xFF);
        }
        return data;
    }

    // A database with one entry that carries one attachment
    std::unique_ptr<Database> databaseWithAttachment(const QByteArray& payload, bool gzip)
    {
        auto db = std::make_unique<Database>();
        db->setCompressionAlgorithm(gzip ? Database::CompressionGZip : Database::CompressionNone);
        auto* entry = new Entry();
        entry->setUuid(QUuid::createUuid());
        entry->setGroup(db->rootGroup());
        entry->setTitle("with attachment");
        entry->attachments()->set("file.bin", payload);
        return db;
    }

    const char* const MinimalHeader = "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>";
} // namespace

void TestXmlImport::initTestCase()
{
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());
}

void TestXmlImport::testRoundTrip()
{
    const auto payload = binaryPayload(5000);
    const auto iconData = binaryPayload(300);
    const auto iconUuid = QUuid::createUuid();

    Database source;
    source.metadata()->setName("Round trip database");
    source.metadata()->setDescription("Description with <markup> & \"quotes\"");
    source.metadata()->addCustomIcon(iconUuid, iconData, "Icon name");
    source.rootGroup()->setNotes("Root notes");

    auto* groupOne = new Group();
    groupOne->setUuid(QUuid::createUuid());
    groupOne->setName("Group One");
    groupOne->setNotes("Group notes <b>& \"quotes\"\nsecond line");
    groupOne->setIcon(iconUuid);
    groupOne->setExpanded(false);
    groupOne->setDefaultAutoTypeSequence("{USERNAME}{ENTER}");
    groupOne->setAutoTypeEnabled(Group::Disable);
    groupOne->setSearchingEnabled(Group::Enable);
    groupOne->customData()->set("group-key", "group-value");
    groupOne->setParent(source.rootGroup());

    auto* nested = new Group();
    nested->setUuid(QUuid::createUuid());
    nested->setName("Nested");
    nested->setParent(groupOne);

    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(nested);
    entry->setTitle("Title \xC3\xBC\xE2\x9C\x93");
    entry->setUsername("user");
    entry->setPassword("original password");
    entry->setUrl("https://example.com/login");
    entry->setNotes("Notes\nwith lines");
    entry->setTags("tag1,tag2");
    entry->setForegroundColor("#112233");
    entry->setBackgroundColor("#445566");
    entry->setOverrideUrl("cmd://run");
    entry->setIcon(iconUuid);
    entry->setExcludeFromReports(true);
    entry->attributes()->set("Custom A", "value A");
    entry->attributes()->set("Secret", "hidden value", true);
    entry->attachments()->set("a.bin", payload);
    entry->attachments()->set("copy.bin", payload);
    entry->attachments()->set("b.txt", "hello");
    entry->setAutoTypeEnabled(false);
    entry->setAutoTypeObfuscation(1);
    entry->setDefaultAutoTypeSequence("{PASSWORD}");
    entry->autoTypeAssociations()->add({"Window *", "{USERNAME}"});
    entry->customData()->set("entry-key", "entry-value");
    auto times = entry->timeInfo();
    times.setExpires(true);
    times.setExpiryTime(QDateTime(QDate(2030, 1, 2), QTime(3, 4, 5), Qt::UTC));
    times.setUsageCount(7);
    entry->setTimeInfo(times);

    // Creates a history item that holds the original state, attachments included
    entry->beginUpdate();
    entry->setPassword("changed password");
    QVERIFY(entry->endUpdate());
    QCOMPARE(entry->historyItems().size(), 1);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("export.xml");
    QVERIFY(exportXml(&source, path));

    Database imported;
    QString error;
    QVERIFY2(imported.import(path, &error), qPrintable(error));

    QCOMPARE(imported.metadata()->name(), QString("Round trip database"));
    QCOMPARE(imported.metadata()->description(), source.metadata()->description());
    QVERIFY(imported.metadata()->hasCustomIcon(iconUuid));
    QCOMPARE(imported.metadata()->customIcon(iconUuid).data, iconData);
    QCOMPARE(imported.rootGroup()->notes(), QString("Root notes"));

    auto* importedGroup = imported.rootGroup()->findGroupByUuid(groupOne->uuid());
    QVERIFY(importedGroup);
    QCOMPARE(importedGroup->name(), groupOne->name());
    QCOMPARE(importedGroup->notes(), groupOne->notes());
    QCOMPARE(importedGroup->iconUuid(), iconUuid);
    QCOMPARE(importedGroup->isExpanded(), false);
    QCOMPARE(importedGroup->defaultAutoTypeSequence(), QString("{USERNAME}{ENTER}"));
    QCOMPARE(importedGroup->autoTypeEnabled(), Group::Disable);
    QCOMPARE(importedGroup->searchingEnabled(), Group::Enable);
    QCOMPARE(importedGroup->customData()->value("group-key"), QString("group-value"));

    auto* importedNested = imported.rootGroup()->findGroupByUuid(nested->uuid());
    QVERIFY(importedNested);
    QCOMPARE(importedNested->parentGroup(), importedGroup);

    auto* importedEntry = imported.rootGroup()->findEntryByUuid(entry->uuid());
    QVERIFY(importedEntry);
    QCOMPARE(importedEntry->group(), importedNested);
    QCOMPARE(importedEntry->title(), entry->title());
    QCOMPARE(importedEntry->username(), QString("user"));
    QCOMPARE(importedEntry->password(), QString("changed password"));
    QCOMPARE(importedEntry->url(), entry->url());
    QCOMPARE(importedEntry->notes(), entry->notes());
    QCOMPARE(importedEntry->tags(), entry->tags());
    QCOMPARE(importedEntry->foregroundColor(), QString("#112233"));
    QCOMPARE(importedEntry->backgroundColor(), QString("#445566"));
    QCOMPARE(importedEntry->overrideUrl(), QString("cmd://run"));
    QCOMPARE(importedEntry->iconUuid(), iconUuid);
    QCOMPARE(importedEntry->excludeFromReports(), true);
    QCOMPARE(importedEntry->customData()->value("entry-key"), QString("entry-value"));

    QCOMPARE(importedEntry->attributes()->value("Custom A"), QString("value A"));
    QCOMPARE(importedEntry->attributes()->value("Secret"), QString("hidden value"));
    QVERIFY(importedEntry->attributes()->isProtected("Secret"));
    QVERIFY(!importedEntry->attributes()->isProtected("Custom A"));

    QCOMPARE(importedEntry->attachments()->keys().size(), 3);
    QCOMPARE(importedEntry->attachments()->value("a.bin"), payload);
    QCOMPARE(importedEntry->attachments()->value("copy.bin"), payload);
    QCOMPARE(importedEntry->attachments()->value("b.txt"), QByteArray("hello"));

    QCOMPARE(importedEntry->autoTypeEnabled(), false);
    QCOMPARE(importedEntry->autoTypeObfuscation(), 1);
    QCOMPARE(importedEntry->defaultAutoTypeSequence(), QString("{PASSWORD}"));
    QCOMPARE(importedEntry->autoTypeAssociations()->getAll().size(), 1);
    QCOMPARE(importedEntry->autoTypeAssociations()->getAll().first().window, QString("Window *"));

    QVERIFY(importedEntry->timeInfo().expires());
    QCOMPARE(importedEntry->timeInfo().expiryTime(), times.expiryTime());
    QCOMPARE(importedEntry->timeInfo().usageCount(), 7);

    // History, including the attachments stored inside the history item
    QCOMPARE(importedEntry->historyItems().size(), 1);
    auto* history = importedEntry->historyItems().first();
    QCOMPARE(history->password(), QString("original password"));
    QCOMPARE(history->attachments()->value("a.bin"), payload);
    QCOMPARE(history->attachments()->value("b.txt"), QByteArray("hello"));
}

void TestXmlImport::testAttachmentsSurviveExport()
{
    const QByteArray payload("PAYLOAD\0\xff\x01", 10);

    Database source;
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(source.rootGroup());
    entry->setTitle("with attachment");
    entry->attachments()->set("file.bin", payload);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("export.xml");
    QVERIFY(exportXml(&source, path));

    // The export itself has to carry the attachment pool, not only the attachment names
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY2(file.readAll().contains("<Binaries>"), "attachment content is missing from the XML export");
    file.close();

    Database imported;
    QString error;
    QVERIFY2(imported.import(path, &error), qPrintable(error));
    auto* importedEntry = imported.rootGroup()->findEntryByUuid(entry->uuid());
    QVERIFY(importedEntry);
    QCOMPARE(importedEntry->attachments()->value("file.bin"), payload);
}

namespace
{
    // Names the parts of an entry that differ, so a failure says what was lost
    QString describeEntryDifference(const Entry* a, const Entry* b);
    QString describeEntryDifference(const Entry* a, const Entry* b)
    {
        QStringList parts;
        if (!a->timeInfo().equals(b->timeInfo(), CompareItemIgnoreMilliseconds)) {
            parts << "times";
        }
        if (*a->attributes() != *b->attributes()) {
            QStringList keys;
            for (const auto& key : a->attributes()->keys()) {
                if (!b->attributes()->hasKey(key)) {
                    keys << key + " (missing)";
                } else if (a->attributes()->value(key) != b->attributes()->value(key)) {
                    auto withoutCr = [](QString s) { return s.remove('\r'); };
                    const bool onlyCarriageReturns =
                        withoutCr(a->attributes()->value(key)) == withoutCr(b->attributes()->value(key));
                    keys << key + (onlyCarriageReturns ? " (value: only carriage returns differ)" : " (value)");
                } else if (a->attributes()->isProtected(key) != b->attributes()->isProtected(key)) {
                    keys << key + " (protect flag)";
                }
            }
            if (a->attributes()->keys().size() != b->attributes()->keys().size()) {
                keys << "key count";
            }
            parts << "attributes[" + keys.join("; ") + "]";
        }
        if (*a->attachments() != *b->attachments()) {
            parts << "attachments";
        }
        if (*a->customData() != *b->customData()) {
            parts << "customData";
        }
        if (*a->autoTypeAssociations() != *b->autoTypeAssociations()) {
            parts << "autoTypeAssociations";
        }
        if (a->historyItems().size() != b->historyItems().size()) {
            parts << "historyCount";
        } else {
            for (int i = 0; i < a->historyItems().size(); ++i) {
                if (!a->historyItems().at(i)->equals(b->historyItems().at(i), CompareItemIgnoreMilliseconds)) {
                    parts << QString("history item %1 (%2)")
                                 .arg(i)
                                 .arg(describeEntryDifference(a->historyItems().at(i), b->historyItems().at(i)));
                }
            }
        }
        if (a->iconNumber() != b->iconNumber() || a->iconUuid() != b->iconUuid()) {
            parts << "icon";
        }
        if (a->foregroundColor() != b->foregroundColor() || a->backgroundColor() != b->backgroundColor()) {
            parts << "colors";
        }
        if (a->tags() != b->tags() || a->overrideUrl() != b->overrideUrl()) {
            parts << "tags/overrideUrl";
        }
        if (a->autoTypeEnabled() != b->autoTypeEnabled() || a->autoTypeObfuscation() != b->autoTypeObfuscation()
            || a->defaultAutoTypeSequence() != b->defaultAutoTypeSequence()) {
            parts << "autoTypeSettings";
        }
        if (a->excludeFromReports() != b->excludeFromReports()
            || a->previousParentGroupUuid() != b->previousParentGroupUuid()) {
            parts << "excludeFromReports/previousParentGroup";
        }
        return parts.isEmpty() ? QString("(unknown part)") : parts.join(", ");
    }

    QString describeGroupDifference(const Group* a, const Group* b)
    {
        QStringList parts;
        if (a->name() != b->name()) {
            parts << "name";
        }
        if (a->notes() != b->notes() || a->tags() != b->tags()) {
            parts << "notes/tags";
        }
        if (a->iconNumber() != b->iconNumber() || a->iconUuid() != b->iconUuid()) {
            parts << "icon";
        }
        if (a->isExpanded() != b->isExpanded()) {
            parts << "expanded";
        }
        if (a->defaultAutoTypeSequence() != b->defaultAutoTypeSequence() || a->autoTypeEnabled() != b->autoTypeEnabled()
            || a->searchingEnabled() != b->searchingEnabled()) {
            parts << "autoType/searching";
        }
        if (a->mergeMode() != b->mergeMode()) {
            parts << "mergeMode";
        }
        if (!a->timeInfo().equals(b->timeInfo(), CompareItemIgnoreMilliseconds)) {
            parts << "times";
        }
        if (*a->customData() != *b->customData()) {
            parts << "customData";
        }
        if (a->previousParentGroupUuid() != b->previousParentGroupUuid()) {
            parts << "previousParentGroup";
        }
        auto uuids = [](const Group* g) {
            QList<QUuid> list;
            for (const auto* c : g->children()) {
                list << c->uuid();
            }
            for (const auto* e : g->entries()) {
                list << e->uuid();
            }
            return list;
        };
        if (uuids(a) != uuids(b)) {
            parts << "children/entries order";
        }
        return parts.isEmpty() ? QString("(unknown part)") : parts.join(", ");
    }

    QString describeMetadataDifference(const Metadata* a, const Metadata* b)
    {
        QStringList parts;
        auto check = [&parts](bool same, const char* name) {
            if (!same) {
                parts << name;
            }
        };
        check(a->name() == b->name(), "name");
        check(a->description() == b->description(), "description");
        check(a->defaultUserName() == b->defaultUserName(), "defaultUserName");
        check(a->color() == b->color(), "color");
        check(a->maintenanceHistoryDays() == b->maintenanceHistoryDays(), "maintenanceHistoryDays");
        check(a->historyMaxItems() == b->historyMaxItems(), "historyMaxItems");
        check(a->historyMaxSize() == b->historyMaxSize(), "historyMaxSize");
        check(a->protectTitle() == b->protectTitle() && a->protectUsername() == b->protectUsername()
                  && a->protectPassword() == b->protectPassword() && a->protectUrl() == b->protectUrl()
                  && a->protectNotes() == b->protectNotes(),
              "memoryProtection");
        check(a->recycleBinEnabled() == b->recycleBinEnabled(), "recycleBinEnabled");
        check((a->recycleBin() ? a->recycleBin()->uuid() : QUuid())
                  == (b->recycleBin() ? b->recycleBin()->uuid() : QUuid()),
              "recycleBin");
        check((a->entryTemplatesGroup() ? a->entryTemplatesGroup()->uuid() : QUuid())
                  == (b->entryTemplatesGroup() ? b->entryTemplatesGroup()->uuid() : QUuid()),
              "entryTemplatesGroup");
        check(*a->customData() == *b->customData(), "customData");
        check(a->customIconsOrder() == b->customIconsOrder(), "customIconsOrder");
        for (const auto& uuid : a->customIconsOrder()) {
            if (!b->hasCustomIcon(uuid) || b->customIcon(uuid).data != a->customIcon(uuid).data
                || b->customIcon(uuid).name != a->customIcon(uuid).name) {
                parts << "customIcons";
                break;
            }
        }
        return parts.isEmpty() ? QString("(unknown part)") : parts.join(", ");
    }
} // namespace

// Compares everything with the library's own equality functions instead of a hand-written list of fields, so a field
// added later to Entry, Group or Metadata that the XML format loses is caught here as well.
void TestXmlImport::testRoundTripKeepsEverything()
{
    const auto iconUuid = QUuid::createUuid();

    Database source;
    auto* meta = source.metadata();
    meta->setName("Everything");
    meta->setDescription("Description\nwith <markup> & \"quotes\"");
    meta->setDefaultUserName("default user");
    meta->setColor("#abcdef");
    meta->setMaintenanceHistoryDays(30);
    meta->setHistoryMaxItems(5);
    meta->setHistoryMaxSize(123456);
    meta->setProtectTitle(true);
    meta->setProtectUsername(true);
    meta->setProtectPassword(false);
    meta->setProtectUrl(true);
    meta->setProtectNotes(true);
    meta->setRecycleBinEnabled(true);
    meta->customData()->set("db-key", "db-value");
    meta->addCustomIcon(
        iconUuid, binaryPayload(200), "Named icon", QDateTime(QDate(2025, 5, 6), QTime(7, 8, 9), Qt::UTC));
    source.rootGroup()->setNotes("Root notes");

    auto* bin = new Group();
    bin->setUuid(QUuid::createUuid());
    bin->setName("Recycle Bin");
    bin->setParent(source.rootGroup());
    meta->setRecycleBin(bin);

    auto* templates = new Group();
    templates->setUuid(QUuid::createUuid());
    templates->setName("Templates");
    templates->setParent(source.rootGroup());
    meta->setEntryTemplatesGroup(templates);

    auto* group = new Group();
    group->setUuid(QUuid::createUuid());
    group->setName("Group with <special> & \"chars\"");
    group->setNotes("Notes");
    group->setTags("group,tags");
    group->setIcon(iconUuid);
    group->setExpanded(false);
    group->setDefaultAutoTypeSequence("{USERNAME}{TAB}{PASSWORD}{ENTER}");
    group->setAutoTypeEnabled(Group::Enable);
    group->setSearchingEnabled(Group::Disable);
    group->customData()->set("group-key", "group-value");
    group->setParent(source.rootGroup());

    // An entry using every feature
    auto* rich = new Entry();
    rich->setUuid(QUuid::createUuid());
    rich->setGroup(group);
    rich->setTitle("Rich entry");
    rich->setUsername("user");
    rich->setPassword("original password");
    rich->setUrl("https://example.com/?a=1&b=2");
    rich->setNotes("Notes");
    rich->setTags("one,two");
    rich->setForegroundColor("#010203");
    rich->setBackgroundColor("#040506");
    rich->setOverrideUrl("cmd://run %1");
    rich->setIcon(iconUuid);
    rich->setExcludeFromReports(true);
    rich->attributes()->set("Plain", "plain value");
    rich->attributes()->set("Protected", "secret value", true);
    rich->attributes()->set("Empty protected", "", true);
    rich->attributes()->set(QString::fromUtf8("K\xC3\xA4y with spaces"), "unicode key");
    rich->attachments()->set("a.bin", binaryPayload(3000));
    rich->attachments()->set("empty.txt", QByteArray());
    rich->setAutoTypeEnabled(false);
    rich->setAutoTypeObfuscation(1);
    rich->setDefaultAutoTypeSequence("{PASSWORD}");
    rich->autoTypeAssociations()->add({"Window *", "{USERNAME}"});
    rich->autoTypeAssociations()->add({"Other window", "{PASSWORD}{ENTER}"});
    rich->customData()->set("entry-key", "entry-value");
    auto times = rich->timeInfo();
    times.setExpires(true);
    times.setExpiryTime(QDateTime(QDate(2031, 2, 3), QTime(4, 5, 6), Qt::UTC));
    times.setUsageCount(42);
    rich->setTimeInfo(times);
    rich->beginUpdate();
    rich->setPassword("second password");
    QVERIFY(rich->endUpdate());
    rich->beginUpdate();
    rich->attachments()->set("added-later.bin", binaryPayload(50));
    QVERIFY(rich->endUpdate());
    QCOMPARE(rich->historyItems().size(), 2);

    // An entry that was moved to the recycle bin
    auto* trashed = new Entry();
    trashed->setUuid(QUuid::createUuid());
    trashed->setGroup(group);
    trashed->setTitle("Deleted entry");
    trashed->setGroup(bin);
    QVERIFY(trashed->previousParentGroupUuid() == group->uuid());

    // Awkward text: Windows line breaks, surrounding whitespace, markup, emoji, empty values
    auto* awkward = new Entry();
    awkward->setUuid(QUuid::createUuid());
    awkward->setGroup(group);
    awkward->setTitle("  leading and trailing  ");
    awkward->setUsername("");
    awkward->setPassword("pa\"ss<w>&rd'");
    awkward->setNotes(QString::fromUtf8("line1\r\nline2\n\tindented \xF0\x9F\x94\x92 &amp; <b>x</b>\r\n"));

    source.addDeletedObject(QUuid::createUuid());
    source.addDeletedObject(QUuid::createUuid());

    // A database read from a file has the per-attribute protection flags of the standard fields set according to the
    // database's memory protection settings. Entries built in memory do not, so bring them in line first.
    auto syncProtectionFlags = [meta](Entry* entry) {
        const QList<QPair<QString, bool>> standard{{EntryAttributes::TitleKey, meta->protectTitle()},
                                                   {EntryAttributes::UserNameKey, meta->protectUsername()},
                                                   {EntryAttributes::PasswordKey, meta->protectPassword()},
                                                   {EntryAttributes::URLKey, meta->protectUrl()},
                                                   {EntryAttributes::NotesKey, meta->protectNotes()}};
        for (const auto& item : standard) {
            if (item.second) {
                entry->attributes()->set(item.first, entry->attributes()->value(item.first), true);
            }
        }
    };
    for (auto* entry : source.rootGroup()->entriesRecursive(true)) {
        syncProtectionFlags(entry);
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("export.xml");
    QVERIFY(exportXml(&source, path));

    Database imported;
    QString error;
    QVERIFY2(imported.import(path, &error), qPrintable(error));

    // Metadata
    QVERIFY2(describeMetadataDifference(meta, imported.metadata()) == "(unknown part)",
             qPrintable("metadata differs: " + describeMetadataDifference(meta, imported.metadata())));

    // Groups
    const auto sourceGroups = source.rootGroup()->groupsRecursive(true);
    QCOMPARE(imported.rootGroup()->groupsRecursive(true).size(), sourceGroups.size());
    for (const auto* g : sourceGroups) {
        const auto* other = g->uuid() == source.rootGroup()->uuid() ? imported.rootGroup()
                                                                    : imported.rootGroup()->findGroupByUuid(g->uuid());
        QVERIFY2(other, qPrintable("group missing: " + g->name()));
        // Not Group::equals(): it compares the custom data pointers instead of their content, so two distinct groups
        // are never equal
        QVERIFY2(describeGroupDifference(g, other) == "(unknown part)",
                 qPrintable("group differs: " + g->name() + " -> " + describeGroupDifference(g, other)));
    }

    // Entries, history included
    const auto sourceEntries = source.rootGroup()->entriesRecursive();
    QCOMPARE(imported.rootGroup()->entriesRecursive().size(), sourceEntries.size());
    for (const auto* e : sourceEntries) {
        const auto* other = imported.rootGroup()->findEntryByUuid(e->uuid());
        QVERIFY2(other, qPrintable("entry missing: " + e->title()));
        QVERIFY2(e->equals(other, CompareItemIgnoreMilliseconds),
                 qPrintable("entry differs: " + e->title() + " -> " + describeEntryDifference(e, other)));
        QCOMPARE(other->group()->uuid(), e->group()->uuid());
    }

    // Deleted objects
    QSet<QUuid> sourceDeleted;
    for (const auto& d : source.deletedObjects()) {
        sourceDeleted.insert(d.uuid);
    }
    QSet<QUuid> importedDeleted;
    for (const auto& d : imported.deletedObjects()) {
        importedDeleted.insert(d.uuid);
    }
    QVERIFY2(sourceDeleted.size() >= 2 && importedDeleted == sourceDeleted, "deleted objects were not kept");
}

void TestXmlImport::testMalformedInput_data()
{
    QTest::addColumn<QByteArray>("xml");

    const auto groupUuid = uuidXml(QUuid::createUuid());
    const auto header = QByteArray(MinimalHeader);

    QTest::newRow("empty") << QByteArray();
    QTest::newRow("not xml") << QByteArray("this is not xml at all");
    QTest::newRow("binary garbage") << QByteArray("\x00\x01\x02\xff\xfe\xfd", 6);
    QTest::newRow("wrong root element") << header + "<Something/>";
    QTest::newRow("no root group") << header + "<KeePassFile><Meta/><Root/></KeePassFile>";
    QTest::newRow("truncated") << header
                                      + QByteArray(
                                          "<KeePassFile><Root><Group><Name>cut off</Name><Entry><String><Key>Title");
    QTest::newRow("two root groups")
        << header
               + "<KeePassFile><Root><Group><Name>a</Name></Group><Group><Name>b</Name></Group></Root>"
                 "</KeePassFile>";
    // Protected values can only be decoded with the inner stream of an encrypted database, which a plain XML file
    // does not have. Must be reported as an error, not dereference a missing stream.
    QTest::newRow("protected string value")
        << header + QByteArray("<KeePassFile><Root><Group><UUID>") + groupUuid.toLatin1()
               + "</UUID><Entry><String><Key>Password</Key><Value Protected=\"True\">AAAAAAAA</Value></String></Entry>"
                 "</Group></Root></KeePassFile>";
    QTest::newRow("protected binary value")
        << header + QByteArray("<KeePassFile><Root><Group><UUID Protected=\"True\">AAAAAAAAAAAAAAAAAAAAAA==</UUID>")
               + "</Group></Root></KeePassFile>";
    QTest::newRow("invalid number") << header
                                           + QByteArray("<KeePassFile><Root><Group><IconID>abc</IconID></Group></Root>"
                                                        "</KeePassFile>");
    QTest::newRow("invalid bool")
        << header + QByteArray("<KeePassFile><Root><Group><IsExpanded>maybe</IsExpanded></Group></Root></KeePassFile>");
}

void TestXmlImport::testMalformedInput()
{
    QFETCH(QByteArray, xml);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("input.xml");
    QVERIFY(writeFile(path, xml));

    Database db;
    QString error;
    QVERIFY2(!db.import(path, &error), "malformed input must be rejected");
    QVERIFY2(!error.isEmpty(), "a rejected import must say why");
}

void TestXmlImport::testMissingFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Database db;
    QString error;
    QVERIFY(!db.import(dir.filePath("does-not-exist.xml"), &error));
    QVERIFY2(!error.isEmpty(), "a missing file must produce an error message");
}

void TestXmlImport::testDuplicateGroupUuidDoesNotCreateCycle_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<int>("groupCount");

    const auto a = uuidXml(QUuid::createUuid()).toLatin1();
    const auto b = uuidXml(QUuid::createUuid()).toLatin1();
    const auto header = QByteArray(MinimalHeader);
    const auto open = QByteArray("<KeePassFile><Root>");
    const auto close = QByteArray("</Root></KeePassFile>");

    // A group nested in a group that carries the same UUID would otherwise become its own parent
    QTest::newRow("child has parent's uuid")
        << header + open + "<Group><UUID>" + a + "</UUID><Name>outer</Name><Group><UUID>" + a
               + "</UUID><Name>inner</Name></Group></Group>" + close
        << 2;
    // A > B > A would otherwise form a loop
    QTest::newRow("grandchild has grandparent's uuid")
        << header + open + "<Group><UUID>" + a + "</UUID><Name>a</Name><Group><UUID>" + b
               + "</UUID><Name>b</Name><Group><UUID>" + a + "</UUID><Name>a again</Name></Group></Group></Group>"
               + close
        << 3;
    QTest::newRow("siblings with the same uuid")
        << header + open + "<Group><UUID>" + a + "</UUID><Name>root</Name><Group><UUID>" + b
               + "</UUID><Name>one</Name></Group><Group><UUID>" + b + "</UUID><Name>two</Name></Group></Group>" + close
        << 3;
}

void TestXmlImport::testDuplicateGroupUuidDoesNotCreateCycle()
{
    QFETCH(QByteArray, xml);
    QFETCH(int, groupCount);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("input.xml");
    QVERIFY(writeFile(path, xml));

    Database db;
    QString error;
    // Either outcome is acceptable as long as no group is lost silently and the tree stays a tree
    if (!db.import(path, &error)) {
        QVERIFY(!error.isEmpty());
        return;
    }

    const auto groups = db.rootGroup()->groupsRecursive(true);
    QCOMPARE(groups.size(), groupCount);
    QSet<QUuid> uuids;
    for (const auto* group : groups) {
        uuids.insert(group->uuid());
    }
    QCOMPARE(uuids.size(), groupCount);
}

void TestXmlImport::testDeepNesting()
{
    // Far deeper than any real database, enough to exhaust the stack of a recursive parser
    const int depth = 200000;
    QByteArray xml(MinimalHeader);
    xml.append("<KeePassFile><Root>");
    for (int i = 0; i < depth; ++i) {
        xml.append("<Group><Name>g</Name>");
    }
    for (int i = 0; i < depth; ++i) {
        xml.append("</Group>");
    }
    xml.append("</Root></KeePassFile>");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("deep.xml");
    QVERIFY(writeFile(path, xml));

    Database db;
    QString error;
    QVERIFY2(!db.import(path, &error), "absurdly deep nesting must be rejected");
    QVERIFY(!error.isEmpty());
}

void TestXmlImport::testReasonableNestingIsAccepted()
{
    // The depth limit must not get in the way of real databases
    const int depth = 100;
    QByteArray xml(MinimalHeader);
    xml.append("<KeePassFile><Root>");
    for (int i = 0; i < depth; ++i) {
        xml.append("<Group><Name>g</Name>");
    }
    for (int i = 0; i < depth; ++i) {
        xml.append("</Group>");
    }
    xml.append("</Root></KeePassFile>");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("nested.xml");
    QVERIFY(writeFile(path, xml));

    Database db;
    QString error;
    QVERIFY2(db.import(path, &error), qPrintable(error));
    QCOMPARE(db.rootGroup()->groupsRecursive(true).size(), depth);
}

void TestXmlImport::testMissingAttachmentContentWarns()
{
    // What older versions exported for a KDBX 4 database: attachment names with a reference but no content
    QByteArray xml(MinimalHeader);
    xml.append("<KeePassFile><Root><Group><UUID>" + uuidXml(QUuid::createUuid()).toLatin1() + "</UUID><Entry><UUID>"
               + uuidXml(QUuid::createUuid()).toLatin1()
               + "</UUID><Binary><Key>file.bin</Key><Value Ref=\"0\"/></Binary></Entry></Group></Root></KeePassFile>");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("old-export.xml");
    QVERIFY(writeFile(path, xml));

    Database db;
    QString error;
    QStringList warnings;
    QVERIFY2(db.import(path, &error, &warnings), qPrintable(error));
    QVERIFY2(!warnings.isEmpty(), "silently importing empty attachments is not acceptable");
}

void TestXmlImport::testImportedGroupKeepsNestedCustomIcons()
{
    // Mirrors what the import wizard does when importing into a group of an open database
    const auto iconUuid = QUuid::createUuid();
    const auto iconData = binaryPayload(64);

    Database source;
    source.metadata()->addCustomIcon(iconUuid, iconData);
    auto* nested = new Group();
    nested->setUuid(QUuid::createUuid());
    nested->setName("nested");
    nested->setParent(source.rootGroup());
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(nested);
    entry->setIcon(iconUuid);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("export.xml");
    QVERIFY(exportXml(&source, path));

    Database imported;
    QString error;
    QVERIFY2(imported.import(path, &error), qPrintable(error));

    Database target;
    auto* importGroup = imported.setRootGroup(new Group());
    auto customIcons = importGroup->customIconsRecursive();
    QVERIFY(customIcons.contains(iconUuid));
    target.metadata()->copyCustomIcons(customIcons, imported.metadata());
    importGroup->setParent(target.rootGroup());

    QVERIFY(target.metadata()->hasCustomIcon(iconUuid));
    QCOMPARE(target.metadata()->customIcon(iconUuid).data, iconData);
}

void TestXmlImport::testEntityExpansion()
{
    // "Billion laughs": must terminate quickly and must not produce a giant value
    QByteArray xml(MinimalHeader);
    xml.append("<!DOCTYPE bomb [<!ENTITY a \"aaaaaaaaaa\">");
    for (int i = 1; i < 10; ++i) {
        const QByteArray name = i == 1 ? QByteArray("a") : QByteArray("e") + QByteArray::number(i - 1);
        const QByteArray prev = "&" + name + ";";
        xml.append("<!ENTITY e" + QByteArray::number(i) + " \"" + prev.repeated(10) + "\">");
    }
    xml.append("]><KeePassFile><Root><Group><Name>&e9;</Name></Group></Root></KeePassFile>");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("bomb.xml");
    QVERIFY(writeFile(path, xml));

    Database db;
    QString error;
    if (db.import(path, &error)) {
        QVERIFY(db.rootGroup()->name().size() < 1000000);
    } else {
        QVERIFY(!error.isEmpty());
    }
}

void TestXmlImport::testExportToDeviceRoundTripsAttachments_data()
{
    // Sizes around 49152 (the planned chunk of raw bytes encoded at a time) and the awkward 0, 1, 2, 3 byte cases.
    QTest::addColumn<int>("size");
    QTest::addColumn<bool>("gzip");
    for (const bool gzip : {true, false}) {
        for (const int size : {0, 1, 2, 3, 49151, 49152, 49153, 98304, 98305, 3000001}) {
            QTest::addRow("%d bytes, %s", size, gzip ? "gzip" : "uncompressed") << size << gzip;
        }
    }
}

// Catches: an attachment that is cut off, padded wrongly or corrupted where one piece of the base64 text ends and
// the next begins.
void TestXmlImport::testExportToDeviceRoundTripsAttachments()
{
    QFETCH(int, size);
    QFETCH(bool, gzip);

    // Random bytes: repetitive data shrinks under gzip to less than one piece and would never reach a boundary
    const auto payload = incompressiblePayload(size);
    auto source = databaseWithAttachment(payload, gzip);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath("export.xml");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QString error;
    QVERIFY2(source->extract(&file, &error), qPrintable(error));
    file.close();

    Database imported;
    QVERIFY2(imported.import(path, &error), qPrintable(error));
    const auto* entry = imported.rootGroup()->entries().value(0);
    QVERIFY2(entry, "the exported entry is missing after import");
    QVERIFY2(entry->attachments()->value("file.bin") == payload, "the attachment content changed");
}

void TestXmlImport::testExportToDeviceReportsWriteFailure_data()
{
    QTest::addColumn<qint64>("failAfter");
    QTest::newRow("the first write fails") << qint64(0);
    QTest::newRow("fails inside the metadata") << qint64(600);
    QTest::newRow("fails inside the attachment") << qint64(2000000);
}

// Catches: an export to a full disk or a closed pipe being reported as a success.
void TestXmlImport::testExportToDeviceReportsWriteFailure()
{
    QFETCH(qint64, failAfter);

    auto source = databaseWithAttachment(incompressiblePayload(3000000), false);

    FailingDevice fullDisk(failAfter);
    QString error;
    QVERIFY2(!source->extract(&fullDisk, &error), "a failed write must not be reported as a success");
    QVERIFY2(error.contains("disk full"), qPrintable(error));
}

namespace
{
    // Exports `source` to a real XML file and reads that file into `imported`, as the import wizard does
    bool exportAndReadBack(Database& source, Database& imported, QTemporaryDir& dir)
    {
        const auto path = dir.filePath("settings.xml");
        QString error;
        return dir.isValid() && exportXml(&source, path) && imported.import(path, &error);
    }

    QStringList sortedIds(const QList<DatabaseSettingChange>& changes)
    {
        QStringList ids;
        for (const auto& change : changes) {
            ids << change.id;
        }
        ids.sort();
        return ids;
    }

    QList<DatabaseSettingChange> only(const QList<DatabaseSettingChange>& changes, const QStringList& ids)
    {
        QList<DatabaseSettingChange> result;
        for (const auto& change : changes) {
            if (ids.contains(change.id)) {
                result << change;
            }
        }
        return result;
    }

    DatabaseSettingChange changeWithId(const QList<DatabaseSettingChange>& changes, const QString& id)
    {
        for (const auto& change : changes) {
            if (change.id == id) {
                return change;
            }
        }
        return {};
    }

    Group* addFolder(Database& db, const QString& name)
    {
        auto* group = new Group();
        group->setUuid(QUuid::createUuid());
        group->setName(name);
        group->setParent(db.rootGroup());
        return group;
    }
} // namespace

// Catches: a setting the XML file carries being silently left out of the list the user is shown. Every setting is
// different from a new database here, so the list has to name every one of them.
void TestXmlImport::testSettingsDiffOffersEverySettingTheFileCarries()
{
    Database open; // a new database, every setting at its default
    Database source;
    auto* meta = source.metadata();
    meta->setName("Work vault");
    meta->setDescription("Team logins");
    meta->setDefaultUserName("teamlead");
    meta->setColor("#112233");
    meta->setMaintenanceHistoryDays(77);
    meta->setMasterKeyChangeRec(90);
    meta->setMasterKeyChangeForce(365);
    meta->setProtectTitle(!open.metadata()->protectTitle());
    meta->setProtectUsername(!open.metadata()->protectUsername());
    meta->setProtectPassword(!open.metadata()->protectPassword());
    meta->setProtectUrl(!open.metadata()->protectUrl());
    meta->setProtectNotes(!open.metadata()->protectNotes());
    meta->setRecycleBinEnabled(!open.metadata()->recycleBinEnabled());
    meta->setHistoryMaxItems(33);
    meta->setHistoryMaxSize(12345678);
    meta->customData()->set("plugin-key", "plugin-value");
    meta->setRecycleBin(addFolder(source, "Old bin"));
    meta->setEntryTemplatesGroup(addFolder(source, "Old templates"));
    meta->addCustomIcon(QUuid::createUuid(), binaryPayload(300), "Icon that no entry uses");
    source.addDeletedObject(QUuid::createUuid());

    QTemporaryDir dir;
    Database imported;
    QVERIFY(exportAndReadBack(source, imported, dir));

    const QStringList expected{"color",
                               "customData:plugin-key",
                               "defaultUserName",
                               "deletedObjects",
                               "description",
                               "entryTemplatesGroup",
                               "historyMaxItems",
                               "historyMaxSize",
                               "maintenanceHistoryDays",
                               "masterKeyChangeForce",
                               "masterKeyChangeRec",
                               "name",
                               "protectNotes",
                               "protectPassword",
                               "protectTitle",
                               "protectUrl",
                               "protectUsername",
                               "recycleBinEnabled",
                               "recycleBinGroup",
                               "unusedCustomIcons"};
    QCOMPARE(sortedIds(DatabaseSettingsDiff::compare(open, imported)), expected);
}

// Catches: settings that are the same being listed anyway (a dialog full of noise), or the program's own bookkeeping
// (last modified time, random slug) being offered as if it were a user setting.
void TestXmlImport::testSettingsDiffIsEmptyWhenNothingDiffers()
{
    Database source;
    auto* meta = source.metadata();
    meta->setName("Same vault");
    meta->setHistoryMaxItems(33);
    meta->customData()->set("plugin-key", "plugin-value");
    meta->setRecycleBin(addFolder(source, "Bin"));
    meta->addCustomIcon(QUuid::createUuid(), binaryPayload(300), "Icon");
    source.addDeletedObject(QUuid::createUuid());

    QTemporaryDir dir;
    Database imported;
    QVERIFY(exportAndReadBack(source, imported, dir));

    const auto changes = DatabaseSettingsDiff::compare(source, imported);
    QVERIFY2(changes.isEmpty(), qPrintable("unexpected: " + sortedIds(changes).join(", ")));
}

// Catches: raw internal values (-1, bytes, true) shown to the user, and protected values shown in plain text.
void TestXmlImport::testSettingsDiffShowsValuesTheWayAPersonReadsThem()
{
    Database open;
    open.metadata()->setName("");
    open.metadata()->setColor("");
    open.metadata()->setHistoryMaxItems(10);
    open.metadata()->setHistoryMaxSize(6 * 1024 * 1024);
    open.metadata()->setMaintenanceHistoryDays(365);
    open.metadata()->setMasterKeyChangeRec(-1);
    open.metadata()->setRecycleBinEnabled(true);

    Database source;
    auto* meta = source.metadata();
    meta->setName("Work vault");
    meta->setColor("#112233");
    meta->setHistoryMaxItems(-1);
    meta->setHistoryMaxSize(12345678);
    meta->setMaintenanceHistoryDays(77);
    meta->setMasterKeyChangeRec(90);
    meta->setRecycleBinEnabled(false);
    meta->customData()->set("KPXC_BROWSER_laptop", "very secret key material");
    meta->customData()->set("long-value", QString(200, 'x'));

    QTemporaryDir dir;
    Database imported;
    QVERIFY(exportAndReadBack(source, imported, dir));
    const auto changes = DatabaseSettingsDiff::compare(open, imported);

    auto name = changeWithId(changes, "name");
    QCOMPARE(name.currentValue, QString("(empty)"));
    QCOMPARE(name.fileValue, QString("Work vault"));
    QCOMPARE(changeWithId(changes, "color").currentValue, QString("(none)"));
    QCOMPARE(changeWithId(changes, "color").fileValue, QString("#112233"));
    QCOMPARE(changeWithId(changes, "historyMaxItems").currentValue, QString("10"));
    QCOMPARE(changeWithId(changes, "historyMaxItems").fileValue, QString("No limit"));
    QCOMPARE(changeWithId(changes, "historyMaxSize").currentValue, QString("6.0 MiB"));
    QCOMPARE(changeWithId(changes, "historyMaxSize").fileValue, QString("11.8 MiB"));
    QCOMPARE(changeWithId(changes, "maintenanceHistoryDays").fileValue, QString("77 days"));
    QCOMPARE(changeWithId(changes, "masterKeyChangeRec").currentValue, QString("Never"));
    QCOMPARE(changeWithId(changes, "masterKeyChangeRec").fileValue, QString("90 days"));
    QCOMPARE(changeWithId(changes, "recycleBinEnabled").currentValue, QString("Yes"));
    QCOMPARE(changeWithId(changes, "recycleBinEnabled").fileValue, QString("No"));

    // The value of a protected item is never shown; the row says what the item is for
    auto browserKey = changeWithId(changes, "customData:KPXC_BROWSER_laptop");
    QCOMPARE(browserKey.fileValue, QString("(hidden)"));
    QVERIFY2(browserKey.label.contains("browser", Qt::CaseInsensitive), qPrintable(browserKey.label));
    QVERIFY(!browserKey.note.isEmpty());
    // Long values are shortened
    auto longValue = changeWithId(changes, "customData:long-value");
    QVERIFY2(longValue.fileValue.endsWith(QChar(0x2026)) && longValue.fileValue.size() <= 61,
             qPrintable(longValue.fileValue));
}

// Catches: choosing one setting changing others as well, or the open database being changed without a choice.
void TestXmlImport::testSettingsApplyChangesOnlyWhatWasChosen()
{
    Database open;
    open.metadata()->setName("Personal");
    open.metadata()->setDescription("Mine");
    open.metadata()->setHistoryMaxItems(10);

    Database source;
    source.metadata()->setName("Work vault");
    source.metadata()->setDescription("Team logins");
    source.metadata()->setHistoryMaxItems(33);

    QTemporaryDir dir;
    Database imported;
    QVERIFY(exportAndReadBack(source, imported, dir));
    const auto changes = DatabaseSettingsDiff::compare(open, imported);

    DatabaseSettingsDiff::apply(open, imported, only(changes, {"name", "historyMaxItems"}), {});

    QCOMPARE(open.metadata()->name(), QString("Work vault"));
    QCOMPARE(open.metadata()->historyMaxItems(), 33);
    QCOMPARE(open.metadata()->description(), QString("Mine"));

    // Nothing chosen: nothing changes
    Database untouched;
    untouched.metadata()->setName("Personal");
    DatabaseSettingsDiff::apply(untouched, imported, {}, {});
    QCOMPARE(untouched.metadata()->name(), QString("Personal"));
}

// Catches: database custom data, unused icons or deletion records being listed but not actually carried over, or
// carried over without being chosen.
void TestXmlImport::testSettingsApplyCarriesCustomDataIconsAndDeletionRecords()
{
    const auto iconUuid = QUuid::createUuid();
    const auto deletedUuid = QUuid::createUuid();

    Database source;
    source.metadata()->customData()->set("plugin-key", "plugin-value");
    source.metadata()->customData()->set("other-key", "other-value");
    source.metadata()->addCustomIcon(iconUuid, binaryPayload(300), "Icon that no entry uses");
    source.addDeletedObject(deletedUuid);

    QTemporaryDir dir;
    Database imported;
    QVERIFY(exportAndReadBack(source, imported, dir));

    Database open;
    const auto changes = DatabaseSettingsDiff::compare(open, imported);
    DatabaseSettingsDiff::apply(
        open, imported, only(changes, {"customData:plugin-key", "unusedCustomIcons", "deletedObjects"}), {});

    QCOMPARE(open.metadata()->customData()->value("plugin-key"), QString("plugin-value"));
    QVERIFY2(!open.metadata()->customData()->contains("other-key"), "an item that was not chosen was carried over");
    QVERIFY(open.metadata()->hasCustomIcon(iconUuid));
    QVERIFY(open.containsDeletedObject(deletedUuid));
}

// Catches: the recycle bin / templates folder being lost because the imported folders were copied with new UUIDs (the
// folder the file points to no longer exists) or because the pointer died when the imported tree was moved.
void TestXmlImport::testSettingsApplyRecycleBinAndTemplatesFolderAfterTheTreeMoved()
{
    Database source;
    auto* originalBin = addFolder(source, "Old bin");
    source.metadata()->setRecycleBin(originalBin);
    source.metadata()->setEntryTemplatesGroup(addFolder(source, "Old templates"));
    const auto originalBinUuid = originalBin->uuid();

    QTemporaryDir dir;
    Database imported;
    QVERIFY(exportAndReadBack(source, imported, dir));

    Database open;
    // The list is made before anything is moved
    const auto changes = DatabaseSettingsDiff::compare(open, imported);

    // What the import does next: take the folders out of the imported database and, because the open database may
    // already use the same UUIDs, import a copy with new ones; the original is deleted
    Group* importGroup = imported.setRootGroup(new Group());
    Group* copy = importGroup->clone(Entry::CloneNewUuid | Entry::CloneIncludeHistory,
                                     Group::CloneNewUuid | Group::CloneIncludeEntries);
    const auto folders = DatabaseSettingsDiff::matchGroups(importGroup, copy);
    delete importGroup;
    copy->setParent(open.rootGroup());

    DatabaseSettingsDiff::apply(open, imported, only(changes, {"recycleBinGroup", "entryTemplatesGroup"}), folders);

    QVERIFY(open.metadata()->recycleBin());
    QCOMPARE(open.metadata()->recycleBin()->name(), QString("Old bin"));
    QVERIFY2(open.metadata()->recycleBin()->uuid() != originalBinUuid, "expected the copy with the new UUID");
    QVERIFY2(open.rootGroup()->findGroupByUuid(open.metadata()->recycleBin()->uuid()),
             "the recycle bin is not a folder of the open database");
    QVERIFY(open.metadata()->entryTemplatesGroup());
    QCOMPARE(open.metadata()->entryTemplatesGroup()->name(), QString("Old templates"));
}

namespace
{
    struct FileForANewDatabase
    {
        QUuid entryUuid;
        QUuid usedIcon;
        QUuid unusedIcon;
        QUuid deletedObject;
        QByteArray attachment;
        // The dates the file's settings were changed on: not "now", so a setter that stamps the current time shows
        QDateTime nameChanged = QDateTime(QDate(2020, 1, 2), QTime(3, 4, 5), Qt::UTC);
        QDateTime descriptionChanged = QDateTime(QDate(2020, 2, 3), QTime(4, 5, 6), Qt::UTC);
        QDateTime defaultUserNameChanged = QDateTime(QDate(2020, 3, 4), QTime(5, 6, 7), Qt::UTC);
        QDateTime recycleBinChanged = QDateTime(QDate(2020, 4, 5), QTime(6, 7, 8), Qt::UTC);
        QDateTime entryTemplatesGroupChanged = QDateTime(QDate(2020, 5, 6), QTime(7, 8, 9), Qt::UTC);
    };

    // A database like the one an XML file comes from: its own root folder, an entry with an attachment and a custom
    // icon, an icon nothing uses, a recycle bin folder and settings that differ from a new database
    void fillSourceForANewDatabase(Database& source, FileForANewDatabase& file)
    {
        file.entryUuid = QUuid::createUuid();
        file.usedIcon = QUuid::createUuid();
        file.unusedIcon = QUuid::createUuid();
        file.deletedObject = QUuid::createUuid();
        file.attachment = incompressiblePayload(5000);

        source.rootGroup()->setName("Work root");
        source.rootGroup()->setNotes("Notes of the root folder");
        auto* sub = addFolder(source, "Sub folder");
        auto* entry = new Entry();
        entry->setUuid(file.entryUuid);
        entry->setGroup(sub);
        entry->setTitle("Work entry");
        entry->attachments()->set("file.bin", file.attachment);
        source.metadata()->addCustomIcon(file.usedIcon, binaryPayload(300), "Icon of the entry");
        entry->setIcon(file.usedIcon);
        source.metadata()->addCustomIcon(file.unusedIcon, binaryPayload(400), "Icon that no entry uses");
        source.metadata()->setRecycleBin(addFolder(source, "Old bin"));
        source.metadata()->setEntryTemplatesGroup(addFolder(source, "Old templates"));
        source.metadata()->setName("Work vault");
        source.metadata()->setDescription("Team logins");
        source.metadata()->setDefaultUserName("teamlead");
        source.metadata()->setColor("#112233");
        source.metadata()->setHistoryMaxItems(33);
        source.metadata()->customData()->set("plugin-key", "plugin-value");
        source.addDeletedObject(file.deletedObject);
        // The setters above stamped the current time; put the file's dates back
        source.metadata()->setNameChanged(file.nameChanged);
        source.metadata()->setDescriptionChanged(file.descriptionChanged);
        source.metadata()->setDefaultUserNameChanged(file.defaultUserNameChanged);
        source.metadata()->setRecycleBinChanged(file.recycleBinChanged);
        source.metadata()->setEntryTemplatesGroupChanged(file.entryTemplatesGroupChanged);
    }

    // A database read from a file has the per-attribute protection flags of the standard fields set according to the
    // database's memory protection settings. Entries built in memory do not, so bring them in line (history included).
    void syncProtectionFlags(Database& db)
    {
        const auto* meta = db.metadata();
        const QList<QPair<QString, bool>> standard{{EntryAttributes::TitleKey, meta->protectTitle()},
                                                   {EntryAttributes::UserNameKey, meta->protectUsername()},
                                                   {EntryAttributes::PasswordKey, meta->protectPassword()},
                                                   {EntryAttributes::URLKey, meta->protectUrl()},
                                                   {EntryAttributes::NotesKey, meta->protectNotes()}};
        for (auto* entry : db.rootGroup()->entriesRecursive(true)) {
            for (const auto& item : standard) {
                if (item.second) {
                    entry->attributes()->set(item.first, entry->attributes()->value(item.first), true);
                }
            }
        }
    }
} // namespace

// Catches: the file's folders and entries (with attachments and icons) not arriving in the new database, a setting the
// user did not choose being applied anyway (here: the name they typed in the new database wizard), or a chosen
// setting being lost.
void TestXmlImport::testNewDatabaseTakesTheFilesContentAndOnlyTheChosenSettings()
{
    Database source;
    FileForANewDatabase file;
    fillSourceForANewDatabase(source, file);
    QTemporaryDir dir;
    Database imported;
    QVERIFY(exportAndReadBack(source, imported, dir));

    // What the new database wizard leaves behind: the name and description the user typed
    Database newDb;
    newDb.metadata()->setName("My typed name");
    newDb.metadata()->setDescription("My typed description");

    const auto changes = DatabaseSettingsDiff::compare(newDb, imported);
    auto chosen = changes;
    for (int i = chosen.size() - 1; i >= 0; --i) {
        if (chosen.at(i).id == "name") {
            chosen.removeAt(i); // the user keeps the name they typed
        }
    }
    QVERIFY2(chosen.size() == changes.size() - 1, "the name was expected to be among the differences");

    const auto notApplied = DatabaseSettingsDiff::adoptImportedTree(newDb, imported, chosen);
    QVERIFY2(notApplied.isEmpty(), qPrintable(notApplied.join(", ")));

    // The tree of the file, root folder included
    QCOMPARE(newDb.rootGroup()->name(), QString("Work root"));
    QCOMPARE(newDb.rootGroup()->notes(), QString("Notes of the root folder"));
    auto* entry = newDb.rootGroup()->findEntryByUuid(file.entryUuid);
    QVERIFY2(entry, "the entry of the file is missing");
    QVERIFY2(entry->attachments()->value("file.bin") == file.attachment, "the attachment changed");
    QVERIFY2(newDb.metadata()->hasCustomIcon(file.usedIcon), "the icon of the entry is missing");

    // Only what was chosen
    QCOMPARE(newDb.metadata()->name(), QString("My typed name"));
    QCOMPARE(newDb.metadata()->description(), QString("Team logins"));
    QCOMPARE(newDb.metadata()->historyMaxItems(), 33);
    QCOMPARE(newDb.metadata()->customData()->value("plugin-key"), QString("plugin-value"));
    QVERIFY(newDb.metadata()->hasCustomIcon(file.unusedIcon));
    QVERIFY(newDb.containsDeletedObject(file.deletedObject));
    QVERIFY(newDb.metadata()->recycleBin());
    QCOMPARE(newDb.metadata()->recycleBin()->name(), QString("Old bin"));
    QVERIFY2(newDb.rootGroup()->findGroupByUuid(newDb.metadata()->recycleBin()->uuid()),
             "the recycle bin is not a folder of the new database");
}

// Catches: a complete copy (the reason to import into a new database) losing a setting when nothing is unticked.
void TestXmlImport::testNewDatabaseGetsEverythingWhenNothingIsUnticked()
{
    Database source;
    FileForANewDatabase file;
    fillSourceForANewDatabase(source, file);
    QTemporaryDir dir;
    Database imported;
    QVERIFY(exportAndReadBack(source, imported, dir));

    Database newDb;
    newDb.metadata()->setName("My typed name");

    const auto changes = DatabaseSettingsDiff::compare(newDb, imported);
    const auto notApplied = DatabaseSettingsDiff::adoptImportedTree(newDb, imported, changes);
    QVERIFY2(notApplied.isEmpty(), qPrintable(notApplied.join(", ")));

    // Nothing the file offered is left behind: comparing again finds no difference
    const auto remaining = DatabaseSettingsDiff::compare(newDb, imported);
    QVERIFY2(remaining.isEmpty(), qPrintable("still different: " + sortedIds(remaining).join(", ")));
}

// Catches a loss that comparing the list again cannot see: the new database has to be a complete copy of the file, down
// to every field of the metadata, the dates the settings were changed on (they decide which side wins when databases
// are merged later), every folder, every entry with its history, and the deletion records. The file was written by a
// database with a rich tree, and the comparison is the one the round trip test uses.
void TestXmlImport::testNewDatabaseIsACompleteCopyOfTheFile()
{
    Database source;
    FileForANewDatabase file;
    fillSourceForANewDatabase(source, file);
    // history on the entry, as a real file has
    auto* entry = source.rootGroup()->findEntryByUuid(file.entryUuid);
    QVERIFY(entry);
    entry->beginUpdate();
    entry->setPassword("changed password");
    QVERIFY(entry->endUpdate());
    QCOMPARE(entry->historyItems().size(), 1);
    syncProtectionFlags(source);

    QTemporaryDir dir;
    Database imported;
    QVERIFY(exportAndReadBack(source, imported, dir));
    // The file really carries the old dates (otherwise this test proves nothing)
    QCOMPARE(imported.metadata()->nameChanged(), file.nameChanged);

    Database newDb;
    newDb.metadata()->setName("My typed name");
    newDb.metadata()->setDescription("My typed description");
    const auto changes = DatabaseSettingsDiff::compare(newDb, imported);
    const auto notApplied = DatabaseSettingsDiff::adoptImportedTree(newDb, imported, changes);
    QVERIFY2(notApplied.isEmpty(), qPrintable(notApplied.join(", ")));

    // Metadata, with the same comparison the round trip test uses
    QVERIFY2(describeMetadataDifference(source.metadata(), newDb.metadata()) == "(unknown part)",
             qPrintable("metadata differs: " + describeMetadataDifference(source.metadata(), newDb.metadata())));
    QCOMPARE(newDb.metadata()->customData()->value("plugin-key"), QString("plugin-value"));

    // The dates the settings were changed on come from the file, not from the moment of the import
    QCOMPARE(newDb.metadata()->nameChanged(), file.nameChanged);
    QCOMPARE(newDb.metadata()->descriptionChanged(), file.descriptionChanged);
    QCOMPARE(newDb.metadata()->defaultUserNameChanged(), file.defaultUserNameChanged);
    QCOMPARE(newDb.metadata()->recycleBinChanged(), file.recycleBinChanged);
    QCOMPARE(newDb.metadata()->entryTemplatesGroupChanged(), file.entryTemplatesGroupChanged);

    // Every folder
    const auto sourceGroups = source.rootGroup()->groupsRecursive(true);
    QCOMPARE(newDb.rootGroup()->groupsRecursive(true).size(), sourceGroups.size());
    for (const auto* group : sourceGroups) {
        const auto* other = group->uuid() == source.rootGroup()->uuid()
                                ? newDb.rootGroup()
                                : newDb.rootGroup()->findGroupByUuid(group->uuid());
        QVERIFY2(other, qPrintable("folder missing: " + group->name()));
        QVERIFY2(describeGroupDifference(group, other) == "(unknown part)",
                 qPrintable("folder differs: " + group->name() + " -> " + describeGroupDifference(group, other)));
    }

    // Every entry, history included
    const auto sourceEntries = source.rootGroup()->entriesRecursive();
    QCOMPARE(newDb.rootGroup()->entriesRecursive().size(), sourceEntries.size());
    for (const auto* sourceEntry : sourceEntries) {
        const auto* other = newDb.rootGroup()->findEntryByUuid(sourceEntry->uuid());
        QVERIFY2(other, qPrintable("entry missing: " + sourceEntry->title()));
        QVERIFY2(sourceEntry->equals(other, CompareItemIgnoreMilliseconds),
                 qPrintable("entry differs: " + sourceEntry->title() + " -> "
                            + describeEntryDifference(sourceEntry, other)));
    }

    // Deletion records
    QSet<QUuid> sourceDeleted;
    for (const auto& record : source.deletedObjects()) {
        sourceDeleted.insert(record.uuid);
    }
    QSet<QUuid> newDeleted;
    for (const auto& record : newDb.deletedObjects()) {
        newDeleted.insert(record.uuid);
    }
    QVERIFY2(!sourceDeleted.isEmpty() && newDeleted == sourceDeleted, "the deletion records were not kept");
}

// Guard: into an OPEN database the user really does change the setting now, so it is dated now. (Into a new database it
// keeps the file's date; the two behaviours must not be mixed up.)
void TestXmlImport::testSettingsAppliedToAnOpenDatabaseAreDatedNow()
{
    Database source;
    FileForANewDatabase file;
    fillSourceForANewDatabase(source, file);
    QTemporaryDir dir;
    Database imported;
    QVERIFY(exportAndReadBack(source, imported, dir));

    Database open;
    open.metadata()->setName("Personal");
    const auto changes = DatabaseSettingsDiff::compare(open, imported);
    DatabaseSettingsDiff::apply(open, imported, only(changes, {"name"}), {});

    QCOMPARE(open.metadata()->name(), QString("Work vault"));
    QVERIFY2(open.metadata()->nameChanged() > file.nameChanged,
             qPrintable("dated " + open.metadata()->nameChanged().toString(Qt::ISODate)));
}
