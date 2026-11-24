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

#include "TestCsvImport.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>

#include "core/AutoTypeAssociations.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/EntryAttachments.h"
#include "core/EntryAttributes.h"
#include "core/Group.h"
#include "core/Totp.h"
#include "crypto/Crypto.h"
#include "format/CsvExporter.h"
#include "gui/MessageBox.h"
#include "gui/csvImport/CsvImportWidget.h"
#include "mock/MockClock.h"

QTEST_MAIN(TestCsvImport)

namespace
{
    // Text built from raw UTF-16 code units, so characters a C++ literal cannot hold can be tested as well
    QString fromUnits(std::initializer_list<ushort> units)
    {
        QString text;
        for (const auto unit : units) {
            text.append(QChar(unit));
        }
        return text;
    }

    QByteArray allByteValues()
    {
        QByteArray data;
        for (int i = 0; i < 256; ++i) {
            data.append(static_cast<char>(i));
        }
        return data;
    }

    bool writeFile(const QString& path, const QByteArray& data)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
    }

    // Exports with the CSV export to a file and imports that file the way the import wizard does: the import form
    // loads the file and builds the database
    QSharedPointer<Database> exportAndImport(const QSharedPointer<Database>& source, QTemporaryDir& dir)
    {
        const auto path = dir.filePath("export.csv");
        CsvExporter exporter;
        if (!exporter.exportDatabase(path, source)) {
            qWarning("export failed: %s", qPrintable(exporter.errorString()));
            return {};
        }
        CsvImportWidget widget;
        widget.load(path);
        return widget.buildDatabase();
    }

    // Text for a failure message: hidden characters written out so the difference can be seen
    QString escaped(const QString& text)
    {
        QString result;
        for (const QChar ch : text) {
            if (ch.unicode() < 0x20 || (ch.unicode() >= 0x7F && ch.unicode() <= 0x9F) || ch.unicode() == 0xFEFF
                || ch.isSurrogate()) {
                result += QString("\u%1").arg(static_cast<uint>(ch.unicode()), 4, 16, QLatin1Char('0'));
            } else {
                result += ch;
            }
        }
        return result.left(80);
    }

    // Times are compared to the second: the CSV (like the database file) keeps no more
    bool sameTime(const QDateTime& a, const QDateTime& b)
    {
        return a.toSecsSinceEpoch() == b.toSecsSinceEpoch();
    }

    // Names everything that is not the same in the two entries, so a failure says what was lost
    QStringList differences(const Entry* a, const Entry* b)
    {
        QStringList d;
        auto check = [&d](bool same, const QString& what) {
            if (!same) {
                d << what;
            }
        };

        auto checkText = [&d](const QString& x, const QString& y, const QString& what) {
            if (x != y) {
                d << QString("%1 (expected \"%2\", got \"%3\")").arg(what, escaped(x), escaped(y));
            }
        };

        check(a->uuid() == b->uuid(), "entry id");
        checkText(a->title(), b->title(), "title");
        checkText(a->username(), b->username(), "user name");
        checkText(a->password(), b->password(), "password");
        checkText(a->url(), b->url(), "url");
        checkText(a->notes(), b->notes(), "notes");
        check(a->tags() == b->tags(), "tags");
        check(a->iconNumber() == b->iconNumber(), "icon");
        check(a->totpSettingsString() == b->totpSettingsString(), "totp");
        check(a->foregroundColor() == b->foregroundColor(), "foreground color");
        check(a->backgroundColor() == b->backgroundColor(), "background color");
        check(a->overrideUrl() == b->overrideUrl(), "override url");
        check(a->excludeFromReports() == b->excludeFromReports(), "exclude from reports");

        // Custom fields: names, values and whether they are hidden
        const auto listA = a->attributes()->customKeys();
        const auto listB = b->attributes()->customKeys();
        const auto keysA = QSet<QString>(listA.cbegin(), listA.cend());
        const auto keysB = QSet<QString>(listB.cbegin(), listB.cend());
        check(keysA == keysB, "custom field names");
        for (const auto& key : keysA) {
            if (keysB.contains(key)) {
                checkText(
                    a->attributes()->value(key), b->attributes()->value(key), "custom field value: " + escaped(key));
                check(a->attributes()->isProtected(key) == b->attributes()->isProtected(key),
                      "custom field hidden flag: " + key);
            }
        }

        const auto attachmentsA = a->attachments()->keys();
        const auto attachmentsB = b->attachments()->keys();
        check(QSet<QString>(attachmentsA.cbegin(), attachmentsA.cend())
                  == QSet<QString>(attachmentsB.cbegin(), attachmentsB.cend()),
              "attachment names");
        for (const auto& name : attachmentsA) {
            check(a->attachments()->value(name) == b->attachments()->value(name), "attachment content: " + name);
        }

        check(a->autoTypeEnabled() == b->autoTypeEnabled(), "auto-type enabled");
        check(a->autoTypeObfuscation() == b->autoTypeObfuscation(), "auto-type obfuscation");
        check(a->defaultAutoTypeSequence() == b->defaultAutoTypeSequence(), "auto-type default sequence");
        const auto assocA = a->autoTypeAssociations()->getAll();
        const auto assocB = b->autoTypeAssociations()->getAll();
        bool sameAssociations = assocA.size() == assocB.size();
        for (int i = 0; sameAssociations && i < assocA.size(); ++i) {
            sameAssociations =
                assocA.at(i).window == assocB.at(i).window && assocA.at(i).sequence == assocB.at(i).sequence;
        }
        check(sameAssociations, "auto-type windows");

        const auto keysDataA = a->customData()->keys();
        for (const auto& key : keysDataA) {
            if (a->customData()->isAutoGenerated(key)) {
                continue;
            }
            check(b->customData()->contains(key) && b->customData()->value(key) == a->customData()->value(key),
                  "custom data: " + key);
        }

        const auto ta = a->timeInfo();
        const auto tb = b->timeInfo();
        check(sameTime(ta.lastModificationTime(), tb.lastModificationTime()), "last modified");
        check(sameTime(ta.creationTime(), tb.creationTime()), "created");
        check(sameTime(ta.lastAccessTime(), tb.lastAccessTime()), "last accessed");
        check(sameTime(ta.locationChanged(), tb.locationChanged()), "location changed");
        check(ta.expires() == tb.expires(), "expires");
        check(sameTime(ta.expiryTime(), tb.expiryTime()), "expiry time");
        check(ta.usageCount() == tb.usageCount(), "usage count");

        // Older versions: the CSV keeps title, user name, password, url, notes, custom fields and the date
        const auto historyA = a->historyItems();
        const auto historyB = b->historyItems();
        check(historyA.size() == historyB.size(),
              QString("number of older versions (expected %1, got %2)").arg(historyA.size()).arg(historyB.size()));
        for (int i = 0; i < qMin(historyA.size(), historyB.size()); ++i) {
            const auto* x = historyA.at(i);
            const auto* y = historyB.at(i);
            const auto label = QString("older version %1: ").arg(i + 1);
            check(x->uuid() == y->uuid(), label + "entry id");
            check(x->title() == y->title(), label + "title");
            check(x->username() == y->username(), label + "user name");
            check(x->password() == y->password(), label + "password");
            check(x->url() == y->url(), label + "url");
            check(x->notes() == y->notes(), label + "notes");
            check(sameTime(x->timeInfo().lastModificationTime(), y->timeInfo().lastModificationTime()), label + "date");
            for (const auto& key : x->attributes()->customKeys()) {
                check(y->attributes()->value(key) == x->attributes()->value(key), label + "custom field: " + key);
                check(y->attributes()->isProtected(key) == x->attributes()->isProtected(key),
                      label + "custom field hidden flag: " + key);
            }
        }
        return d;
    }

    // An entry that uses every part of the entry editor (the main page, Advanced, Auto-Type, Properties, History and
    // the browser settings)
    Entry* addRichEntry(Group* group, const QString& title)
    {
        auto* entry = new Entry();
        entry->setUuid(QUuid::createUuid());
        entry->setGroup(group);
        entry->setTitle(title);
        entry->setUsername("user name");
        entry->setPassword("p@ss,\"word\"");
        entry->setUrl("https://example.com/?a=1&b=2");
        entry->setNotes("line one\nline two, with a comma and \"quotes\"");
        entry->setTags("alpha,beta");
        entry->setIcon(7);
        entry->setTotp(Totp::createSettings("DFDF", Totp::DEFAULT_DIGITS, Totp::DEFAULT_STEP));

        // Advanced: custom fields (one hidden) and attachments
        entry->attributes()->set("Plain field", "plain value");
        entry->attributes()->set("Hidden field", "hidden value", true);
        entry->attributes()->set("Name, with \"punctuation\"", "odd name");
        entry->attachments()->set("all bytes.bin", allByteValues());
        entry->attachments()->set("empty.txt", QByteArray());

        // Auto-Type
        entry->setAutoTypeEnabled(false);
        entry->setAutoTypeObfuscation(1);
        entry->setDefaultAutoTypeSequence("{USERNAME}{TAB}{PASSWORD}{ENTER}");
        entry->autoTypeAssociations()->add({"Window *", "{USERNAME}"});
        entry->autoTypeAssociations()->add({"Other window", "{PASSWORD}{ENTER}"});

        // Properties, and browser settings
        entry->setForegroundColor("#010203");
        entry->setBackgroundColor("#040506");
        entry->setOverrideUrl("cmd://run %1");
        entry->setExcludeFromReports(true);
        entry->customData()->set("BrowserSettings", "{\"skipAutoSubmit\":true}");

        // History: two older versions with their own dates, one with a hidden custom field
        auto* clock = new MockClock(2020, 1, 1, 8, 0, 0);
        MockClock::setup(clock);
        entry->beginUpdate();
        entry->setPassword("first password");
        entry->attributes()->set("Hidden field", "older hidden value", true);
        entry->endUpdate();
        clock->advanceDay(3);
        entry->beginUpdate();
        entry->setPassword("second password");
        entry->setNotes("older notes");
        entry->endUpdate();
        MockClock::teardown();
        entry->setPassword("p@ss,\"word\"");
        entry->setNotes("line one\nline two, with a comma and \"quotes\"");
        entry->attributes()->set("Hidden field", "hidden value", true);

        TimeInfo times;
        times.setCreationTime(QDateTime(QDate(2019, 5, 6), QTime(7, 8, 9), Qt::UTC));
        times.setLastModificationTime(QDateTime(QDate(2024, 2, 3), QTime(4, 5, 6), Qt::UTC));
        times.setLastAccessTime(QDateTime(QDate(2025, 3, 4), QTime(5, 6, 7), Qt::UTC));
        times.setLocationChanged(QDateTime(QDate(2021, 9, 8), QTime(7, 6, 5), Qt::UTC));
        times.setExpires(true);
        times.setExpiryTime(QDateTime(QDate(2031, 2, 3), QTime(4, 5, 6), Qt::UTC));
        times.setUsageCount(42);
        entry->setTimeInfo(times);
        return entry;
    }
} // namespace

void TestCsvImport::initTestCase()
{
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());
}

// Catches: a column of the CSV export that the import does not read back, so that data is lost when a person
// exports to CSV and imports that file again. Every tab of the entry editor is covered.
void TestCsvImport::testEveryEntryFieldSurvivesTheCsvRoundTrip()
{
    auto source = QSharedPointer<Database>::create();
    auto* work = new Group();
    work->setName("Work");
    work->setParent(source->rootGroup());
    auto* sub = new Group();
    sub->setName("Sub folder");
    sub->setParent(work);
    auto* rich = addRichEntry(sub, "Rich entry");

    // A plain entry next to it, in another folder
    auto* plain = new Entry();
    plain->setUuid(QUuid::createUuid());
    plain->setGroup(work);
    plain->setTitle("Plain entry");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto imported = exportAndImport(source, dir);
    QVERIFY2(imported, "the import produced no database");

    QCOMPARE(imported->rootGroup()->entriesRecursive().size(), 2);
    for (const auto* original : {static_cast<const Entry*>(rich), static_cast<const Entry*>(plain)}) {
        const auto* found = imported->rootGroup()->findEntryByUuid(original->uuid());
        QVERIFY2(found, qPrintable("entry not found by its id: " + original->title()));
        const auto problems = differences(original, found);
        QVERIFY2(problems.isEmpty(), qPrintable(original->title() + ": " + problems.join("; ")));
        QCOMPARE(found->group()->name(), original->group()->name());
    }
}

void TestCsvImport::testTrickyText_data()
{
    QTest::addColumn<QString>("value");

    QTest::newRow("a hidden control character") << fromUnits({'a', 'b', 0x01, 'c', 'd', 0x7F, 'e', 'f'});
    QTest::newRow("quotes, commas and line breaks") << QString("a \"quoted\", value\nwith\nline breaks");
    QTest::newRow("Windows line breaks") << QString("one\r\ntwo\r\nthree");
    QTest::newRow("leading and trailing spaces") << QString("  padded  ");
    QTest::newRow("a tab and a backslash") << QString("tab\there \\ backslash");
    QTest::newRow("markup") << QString("<b>&amp;</b>");
    QTest::newRow("emoji and accents") << QString::fromUtf8("caf\xC3\xA9 \xF0\x9F\x94\x92");
    QTest::newRow("starts with U+FEFF") << fromUnits({0xFEFF, 'a', 'b'});
    QTest::newRow("a NUL character") << fromUnits({'a', 0x00, 'b'});
    QTest::newRow("only a quote") << QString("\"");
    QTest::newRow("looks like JSON") << QString("{\"a\":[1,2,3]}");
    QTest::newRow("half of an emoji") << fromUnits({'a', 0xD83D, 'b'});
}

// Catches: text that the CSV export and import do not give back unchanged. Passwords matter most.
void TestCsvImport::testTrickyText()
{
    QFETCH(QString, value);

    auto source = QSharedPointer<Database>::create();
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(source->rootGroup());
    entry->setTitle("Tricky " + value);
    entry->setUsername(value);
    entry->setPassword(value);
    entry->setUrl(value);
    entry->setNotes(value);
    entry->setOverrideUrl(value);
    entry->attributes()->set("field " + value, value);
    entry->attributes()->set("hidden " + value, value, true);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto imported = exportAndImport(source, dir);
    QVERIFY2(imported, "the import produced no database");
    const auto* found = imported->rootGroup()->findEntryByUuid(entry->uuid());
    QVERIFY2(found, "the entry is missing");
    const auto problems = differences(entry, found);
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join("; ")));
}

namespace
{
    bool hasLoneSurrogate(const QString& text)
    {
        for (int i = 0; i < text.size(); ++i) {
            const auto unit = text.at(i).unicode();
            if (QChar::isHighSurrogate(unit)) {
                if (i + 1 < text.size() && QChar::isLowSurrogate(text.at(i + 1).unicode())) {
                    ++i;
                    continue;
                }
                return true;
            }
            if (QChar::isLowSurrogate(unit)) {
                return true;
            }
        }
        return false;
    }
} // namespace

// Catches: half of an emoji (which UTF-8 cannot carry) lost in the places a plain CSV cell holds text: the folder
// name, the tags and the colours as well as the fields of the main page. Also that the plain cells stay valid text.
void TestCsvImport::testHalfAnEmojiInEveryTextColumn()
{
    const auto half = fromUnits({'x', 0xD83D, 'y', 0xDC00});
    auto source = QSharedPointer<Database>::create();
    auto* folder = new Group();
    folder->setName("Folder " + half);
    folder->setParent(source->rootGroup());
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(folder);
    entry->setTitle("t" + half);
    entry->setTags("tag" + half);
    entry->setForegroundColor("#0" + half);
    entry->setBackgroundColor("#1" + half);
    entry->setOverrideUrl("url" + half);
    entry->setTotp(Totp::createSettings("DFDF", Totp::DEFAULT_DIGITS, Totp::DEFAULT_STEP));

    CsvExporter exporter;
    QVERIFY2(!hasLoneSurrogate(exporter.exportDatabase(source)), "the plain cells must be valid text");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto imported = exportAndImport(source, dir);
    QVERIFY2(imported, "the import produced no database");
    const auto* found = imported->rootGroup()->findEntryByUuid(entry->uuid());
    QVERIFY2(found, "the entry is missing");
    QCOMPARE(escaped(found->group()->name()), escaped(folder->name()));
    const auto problems = differences(entry, found);
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join("; ")));
}

// Catches: the extra column filled for ordinary entries (it must stay empty unless a cell holds half an emoji), and a
// column that moved (the heading is how the import finds it).
void TestCsvImport::testExactTextColumnStaysEmptyForOrdinaryText()
{
    auto source = QSharedPointer<Database>::create();
    auto* rich = addRichEntry(source->rootGroup(), "Ordinary " + QString::fromUtf8("caf\xC3\xA9 \xF0\x9F\x94\x92"));
    Q_UNUSED(rich);

    CsvExporter exporter;
    const auto csv = exporter.exportDatabase(source);
    const auto header = csv.left(csv.indexOf('\n'));
    QVERIFY2(header.endsWith(",\"Custom Data\",\"Exact Text\""), qPrintable(header));
    // (the notes of the entry hold a line break inside a cell, so the file is not split into lines here)
    QVERIFY2(csv.endsWith(",\"\"\n"), qPrintable("the last cell is not empty: " + csv.right(60)));
}

// Not a test of this program: tells which unusual characters Qt's JSON writer and reader keep. The export writes the
// custom fields as JSON, so a character they lose is lost in the CSV.
void TestCsvImport::testWhichCharactersTheJsonStepKeeps_data()
{
    QTest::addColumn<QString>("value");
    // 0xD83D and 0xDC00: half of an emoji (a lone surrogate), first half and second half
    for (const ushort unit :
         {0xFEFF, 0xFFFE, 0xFFFF, 0x2028, 0x2029, 0x0085, 0x007F, 0x0001, 0x0000, 0x00A0, 0x200B, 0xD83D, 0xDC00}) {
        QTest::newRow(qPrintable(QString("U+%1 at the start").arg(unit, 4, 16, QLatin1Char('0'))))
            << (QString(QChar(unit)) + "ab");
        QTest::newRow(qPrintable(QString("U+%1 in the middle").arg(unit, 4, 16, QLatin1Char('0'))))
            << (QString("a") + QChar(unit) + "b");
        QTest::newRow(qPrintable(QString("U+%1 at the end").arg(unit, 4, 16, QLatin1Char('0'))))
            << (QString("ab") + QChar(unit));
    }
}

void TestCsvImport::testWhichCharactersTheJsonStepKeeps()
{
    QFETCH(QString, value);
    QJsonObject object;
    object.insert(value, value);
    const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
    // as the export writes it: text, then as the import reads it: bytes again
    const auto text = QString::fromUtf8(bytes);
    QJsonParseError error;
    const auto parsed = QJsonDocument::fromJson(text.toUtf8(), &error);
    QVERIFY2(error.error == QJsonParseError::NoError, qPrintable(error.errorString()));
    QVERIFY2(parsed.object().contains(value),
             qPrintable("the name changed: " + escaped(parsed.object().keys().value(0))));
    // Qt drops a U+FEFF that starts a value. CsvExporter writes it as the JSON escape instead (checked by
    // testTrickyText, "starts with U+FEFF"); if Qt ever stops dropping it, this passes unexpectedly and the escape can
    // go.
    QEXPECT_FAIL("U+feff at the start", "Qt's JSON reader drops it; the CSV export works around that", Abort);
    QCOMPARE(escaped(parsed.object().value(value).toString()), escaped(value));
}
