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

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>

#include "core/AutoTypeAssociations.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/EntryAttachments.h"
#include "core/EntryAttributes.h"
#include "core/Group.h"
#include "core/Merger.h"
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
            // Everything else an older version holds
            check(x->iconNumber() == y->iconNumber(), label + "icon");
            check(x->tags() == y->tags(), label + "tags");
            check(x->foregroundColor() == y->foregroundColor(), label + "foreground color");
            check(x->backgroundColor() == y->backgroundColor(), label + "background color");
            check(x->overrideUrl() == y->overrideUrl(), label + "override url");
            check(x->excludeFromReports() == y->excludeFromReports(), label + "exclude from reports");
            check(x->timeInfo().expires() == y->timeInfo().expires(), label + "expires");
            check(sameTime(x->timeInfo().expiryTime(), y->timeInfo().expiryTime()), label + "expiry time");
            check(x->timeInfo().usageCount() == y->timeInfo().usageCount(), label + "usage count");
            check(sameTime(x->timeInfo().creationTime(), y->timeInfo().creationTime()), label + "created");
            check(sameTime(x->timeInfo().lastAccessTime(), y->timeInfo().lastAccessTime()), label + "last accessed");
            check(sameTime(x->timeInfo().locationChanged(), y->timeInfo().locationChanged()),
                  label + "location changed");
            check(x->autoTypeEnabled() == y->autoTypeEnabled(), label + "auto-type enabled");
            check(x->autoTypeObfuscation() == y->autoTypeObfuscation(), label + "auto-type obfuscation");
            check(x->defaultAutoTypeSequence() == y->defaultAutoTypeSequence(), label + "auto-type default sequence");
            bool sameWindows = x->autoTypeAssociations()->getAll().size() == y->autoTypeAssociations()->getAll().size();
            for (int w = 0; sameWindows && w < x->autoTypeAssociations()->getAll().size(); ++w) {
                sameWindows =
                    x->autoTypeAssociations()->getAll().at(w).window == y->autoTypeAssociations()->getAll().at(w).window
                    && x->autoTypeAssociations()->getAll().at(w).sequence
                           == y->autoTypeAssociations()->getAll().at(w).sequence;
            }
            check(sameWindows, label + "auto-type windows");
            for (const auto& key : x->customData()->keys()) {
                if (!x->customData()->isAutoGenerated(key)) {
                    check(y->customData()->contains(key) && y->customData()->value(key) == x->customData()->value(key),
                          label + "custom data: " + key);
                }
            }
            const auto olderNamesX = x->attachments()->keys();
            const auto olderNamesY = y->attachments()->keys();
            check(QSet<QString>(olderNamesX.cbegin(), olderNamesX.cend())
                      == QSet<QString>(olderNamesY.cbegin(), olderNamesY.cend()),
                  label + "attachment names");
            for (const auto& name : olderNamesX) {
                check(x->attachments()->value(name) == y->attachments()->value(name), label + "attachment: " + name);
            }
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
        // (an older version holds everything the entry had at the time: expiry and use count, then a changed icon,
        // tags, colours, attachment, ...)
        TimeInfo early;
        early.setExpires(true);
        early.setExpiryTime(QDateTime(QDate(2030, 1, 2), QTime(3, 4, 5), Qt::UTC));
        early.setUsageCount(7);
        entry->setTimeInfo(early);
        entry->beginUpdate();
        entry->setPassword("first password");
        entry->attributes()->set("Hidden field", "older hidden value", true);
        entry->setIcon(3);
        entry->setTags("old tag");
        entry->setForegroundColor("#aa0000");
        entry->setBackgroundColor("#00bb00");
        entry->setOverrideUrl("cmd://older");
        entry->setExcludeFromReports(false);
        entry->attachments()->set("all bytes.bin", QByteArray("an older attachment"));
        entry->customData()->set("OlderData", "older value");
        entry->setAutoTypeObfuscation(0);
        entry->setDefaultAutoTypeSequence("{PASSWORD}");
        entry->endUpdate();
        clock->advanceDay(3);
        entry->beginUpdate();
        entry->setPassword("second password");
        entry->setNotes("older notes");
        entry->endUpdate();
        MockClock::teardown();
        entry->setIcon(7);
        entry->setTags("alpha,beta");
        entry->setForegroundColor("#010203");
        entry->setBackgroundColor("#040506");
        entry->setOverrideUrl("cmd://run %1");
        entry->setExcludeFromReports(true);
        entry->attachments()->set("all bytes.bin", allByteValues());
        entry->customData()->remove("OlderData");
        entry->setAutoTypeObfuscation(1);
        entry->setDefaultAutoTypeSequence("{USERNAME}{TAB}{PASSWORD}{ENTER}");
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

namespace
{
    // One CSV line from cells (every cell quoted, quotes doubled)
    QByteArray csvLine(const QStringList& cells)
    {
        QStringList quoted;
        for (const auto& cell : cells) {
            quoted << "\"" + QString(cell).replace("\"", "\"\"") + "\"";
        }
        return quoted.join(",").toUtf8() + "\n";
    }

    // Loads a hand-written CSV file into the import form, as the import wizard does. The form stays with the caller so
    // that its settings can be changed before the database is built and its problems read afterwards.
    bool loadText(CsvImportWidget& widget, QTemporaryDir& dir, const QByteArray& text)
    {
        const auto path = dir.filePath("hand-written.csv");
        if (!writeFile(path, text)) {
            return false;
        }
        widget.load(path);
        return true;
    }

    const Entry* entryTitled(const QSharedPointer<Database>& db, const QString& title)
    {
        for (const auto* entry : db->rootGroup()->entriesRecursive()) {
            if (entry->title() == title) {
                return entry;
            }
        }
        return nullptr;
    }
} // namespace

// Catches: columns of another program's file that the form has no field for being dropped, or kept under a name that
// replaces a field the entry already has. The box can be unticked to leave them out.
void TestCsvImport::testUnusedColumnsAreKeptAsCustomFields()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CsvImportWidget widget;
    QVERIFY(loadText(widget,
                     dir,
                     csvLine({"Group", "Title", "Username", "Password", "Folder", "Favorite", "Empty", "Title"})
                         + csvLine({"Web", "Shop", "me", "pw", "Home", "true", "", "second title column"})));
    auto db = widget.buildDatabase();
    QVERIFY(db);
    const auto* entry = entryTitled(db, "Shop");
    QVERIFY(entry);
    // an asterisk marks a column that was not recognised; the second "Title" column cannot replace the entry's title
    QCOMPARE(entry->attributes()->value("Folder *"), QString("Home"));
    QCOMPARE(entry->attributes()->value("Favorite *"), QString("true"));
    QVERIFY2(!entry->attributes()->hasKey("Empty *"), "an empty cell must not make a custom field");
    QCOMPARE(entry->title(), QString("Shop"));
    QCOMPARE(entry->attributes()->value("Title *"), QString("second title column"));
    QVERIFY2(widget.findChild<QLabel*>("labelKeepOtherColumnsNote"), "the form must explain the asterisk");
    QVERIFY2(widget.problems().isEmpty(), qPrintable(widget.problems().join("; ")));

    // Unticked: the columns are left out
    auto* keep = widget.findChild<QCheckBox*>("checkBoxKeepOtherColumns");
    QVERIFY(keep);
    QVERIFY2(keep->isChecked(), "the columns must be kept unless the person unticks the box");
    keep->setChecked(false);
    db = widget.buildDatabase();
    QVERIFY(db);
    entry = entryTitled(db, "Shop");
    QVERIFY(entry);
    QVERIFY(entry->attributes()->customKeys().isEmpty());
}

// Catches: a file without field names losing its unused columns, or the names differing from the form's drop-down
// lists ("Column 0", "Column 1", ...).
void TestCsvImport::testUnusedColumnsOfAFileWithoutFieldNames()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CsvImportWidget widget;
    QVERIFY(loadText(widget, dir, csvLine({"Web", "Shop", "me", "pw", "extra one", "extra two"})));
    widget.findChild<QCheckBox*>("checkBoxFieldNames")->setChecked(false);
    widget.findChild<QComboBox*>("groupCombo")->setCurrentIndex(1);
    widget.findChild<QComboBox*>("titleCombo")->setCurrentIndex(2);
    widget.findChild<QComboBox*>("usernameCombo")->setCurrentIndex(3);
    widget.findChild<QComboBox*>("passwordCombo")->setCurrentIndex(4);
    const auto db = widget.buildDatabase();
    QVERIFY(db);
    const auto* entry = entryTitled(db, "Shop");
    QVERIFY(entry);
    QCOMPARE(entry->password(), QString("pw"));
    QCOMPARE(entry->attributes()->value("Column 4 *"), QString("extra one"));
    QCOMPARE(entry->attributes()->value("Column 5 *"), QString("extra two"));
}

// Catches: files from earlier versions no longer importing: the 21 columns this program wrote before the newest ones
// were added, and the ten columns of the original export.
void TestCsvImport::testFilesOfEarlierVersionsStillImport()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    {
        CsvImportWidget widget;
        const auto header =
            csvLine({"Group",       "Title",         "Username",         "Password",    "URL",  "Notes",
                     "TOTP",        "Icon",          "Last Modified",    "Created",     "Tags", "Expires",
                     "Expiry Time", "Last Accessed", "Location Changed", "Usage Count", "UUID", "Custom Attributes",
                     "Auto-Type",   "Attachments",   "History"});
        const auto row =
            csvLine({"Web",
                     "Shop",
                     "me",
                     "pw",
                     "https://x",
                     "n",
                     "",
                     "5",
                     "2020-01-02T03:04:05Z",
                     "2019-01-02T03:04:05Z",
                     "a",
                     "true",
                     "2031-02-03T04:05:06Z",
                     "2025-03-04T05:06:07Z",
                     "2021-09-08T07:06:05Z",
                     "42",
                     "00112233445566778899aabbccddeeff",
                     "{\"Plain\":\"v\"}",
                     "{\"enabled\":false,\"obfuscation\":1,\"defaultSequence\":\"{PASSWORD}\",\"associations\":[]}",
                     "{\"a.txt\":\"aGk=\"}",
                     "[{\"title\":\"Old\",\"username\":\"u\",\"password\":\"old pw\",\"url\":\"\",\"notes\":\"\","
                     "\"modified\":\"2018-05-06T07:08:09Z\"}]"});
        QVERIFY(loadText(widget, dir, header + row));
        const auto db = widget.buildDatabase();
        QVERIFY(db);
        QVERIFY2(widget.problems().isEmpty(), qPrintable(widget.problems().join("; ")));
        const auto* entry = entryTitled(db, "Shop");
        QVERIFY(entry);
        QCOMPARE(entry->uuidToHex(), QString("00112233445566778899aabbccddeeff"));
        QCOMPARE(entry->attributes()->value("Plain"), QString("v"));
        QVERIFY(!entry->attributes()->isProtected("Plain"));
        QCOMPARE(entry->attachments()->value("a.txt"), QByteArray("hi"));
        QVERIFY(!entry->autoTypeEnabled());
        QCOMPARE(entry->autoTypeObfuscation(), 1);
        QCOMPARE(entry->defaultAutoTypeSequence(), QString("{PASSWORD}"));
        QCOMPARE(entry->iconNumber(), 5);
        QCOMPARE(entry->timeInfo().usageCount(), 42);
        QVERIFY(entry->timeInfo().expires());
        QCOMPARE(entry->timeInfo().lastModificationTime().toSecsSinceEpoch(),
                 QDateTime(QDate(2020, 1, 2), QTime(3, 4, 5), Qt::UTC).toSecsSinceEpoch());
        QCOMPARE(entry->historyItems().size(), 1);
        QCOMPARE(entry->historyItems().at(0)->password(), QString("old pw"));
    }

    {
        CsvImportWidget widget;
        const auto header = csvLine(
            {"Group", "Title", "Username", "Password", "URL", "Notes", "TOTP", "Icon", "Last Modified", "Created"});
        const auto row =
            csvLine({"Web", "Plain", "me", "pw", "https://y", "note", "", "3", "1577934245", "2019-01-02T03:04:05Z"});
        QVERIFY(loadText(widget, dir, header + row));
        const auto db = widget.buildDatabase();
        QVERIFY(db);
        QVERIFY2(widget.problems().isEmpty(), qPrintable(widget.problems().join("; ")));
        const auto* entry = entryTitled(db, "Plain");
        QVERIFY(entry);
        QCOMPARE(entry->url(), QString("https://y"));
        QCOMPARE(entry->iconNumber(), 3);
        QCOMPARE(entry->timeInfo().lastModificationTime().toSecsSinceEpoch(), qint64(1577934245));
        QVERIFY(entry->attributes()->customKeys().isEmpty());
        QVERIFY(entry->historyItems().isEmpty());
    }
}

// Catches: a cell that cannot be understood being dropped without a word, or not reported. Each one must be kept as a
// custom field with its original text and named in the problems the person is shown.
void TestCsvImport::testCellsThatCannotBeUnderstoodAreKeptAndReported()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CsvImportWidget widget;
    const QString id = "00112233445566778899aabbccddeeff";
    const auto header = csvLine({"Group",
                                 "Title",
                                 "Username",
                                 "Password",
                                 "UUID",
                                 "Expiry Time",
                                 "Custom Attributes",
                                 "Attachments",
                                 "Protected Attributes"});
    const auto rows =
        csvLine({"Web",
                 "bad cells",
                 "",
                 "",
                 id,
                 "not a date",
                 "{not json",
                 "{\"x.bin\":\"***not base64***\"}",
                 "[not json"})
        + csvLine({"Web", "same id", "", "", id, "", "{\"Title\":\"clash\",\"Notes\":\"clash2\"}", "", ""})
        + csvLine({"Web", "bad id", "", "", "xyz", "", "", "", ""});
    QVERIFY(loadText(widget, dir, header + rows));
    const auto db = widget.buildDatabase();
    QVERIFY(db);
    QCOMPARE(db->rootGroup()->entriesRecursive().size(), 3);

    const auto* bad = entryTitled(db, "bad cells");
    QVERIFY(bad);
    QCOMPARE(bad->uuidToHex(), id);
    QCOMPARE(bad->attributes()->value("Custom Attributes (CSV text that was not understood)"), QString("{not json"));
    QCOMPARE(bad->attributes()->value("Attachment x.bin (CSV text that was not understood)"),
             QString("***not base64***"));
    QCOMPARE(bad->attributes()->value("Protected Attributes (CSV text that was not understood)"), QString("[not json"));

    const auto* same = entryTitled(db, "same id");
    QVERIFY(same);
    QVERIFY2(!same->uuid().isNull() && same->uuidToHex() != id, "a repeated id must give the entry a new one");
    QCOMPARE(same->attributes()->value("Title (CSV)"), QString("clash"));
    QCOMPARE(same->attributes()->value("Notes (CSV)"), QString("clash2"));

    const auto* invalid = entryTitled(db, "bad id");
    QVERIFY(invalid);
    QVERIFY(!invalid->uuid().isNull());

    const auto problems = widget.problems().join("\n");
    for (const QString& expected : QStringList{"Row 2 (bad cells)",
                                               "not a valid date",
                                               "attachment \"x.bin\"",
                                               "Row 3 (same id)",
                                               "appears more than once",
                                               "Row 4 (bad id)",
                                               "is not valid",
                                               "custom field \"Title\""}) {
        QVERIFY2(problems.contains(expected), qPrintable("not reported: " + expected + "\nreported:\n" + problems));
    }
}

// Catches: the exact text of a cell being lost when its column cannot be read or the field was taken from another
// column
void TestCsvImport::testExactTextThatCannotBeUsed()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    {
        CsvImportWidget widget;
        QVERIFY(loadText(widget,
                         dir,
                         csvLine({"Group", "Title", "Username", "Password", "Exact Text"})
                             + csvLine({"Web", "readable", "u", "pw?", "{\"Password\":\"pw\\ud83d\"}"})
                             + csvLine({"Web", "unreadable", "u", "pw?", "not json"})));
        const auto db = widget.buildDatabase();
        QVERIFY(db);
        QCOMPARE(entryTitled(db, "readable")->password(), QString("pw") + QChar(0xD83D));
        const auto* unreadable = entryTitled(db, "unreadable");
        QCOMPARE(unreadable->password(), QString("pw?"));
        QCOMPARE(unreadable->attributes()->value("Exact Text (CSV text that was not understood)"), QString("not json"));
        QVERIFY2(widget.problems().join("\n").contains("exact text of some cells could not be read"),
                 qPrintable(widget.problems().join("\n")));
    }

    {
        // The person took the password from another column: the exact text belongs to the column of the same name and
        // must not replace it; the unused "Password" column is kept
        CsvImportWidget widget;
        QVERIFY(loadText(widget,
                         dir,
                         csvLine({"Group", "Title", "Username", "Password", "Other", "Exact Text"})
                             + csvLine({"Web", "remapped", "u", "plain", "other pw", "{\"Password\":\"exact\"}"})));
        widget.findChild<QComboBox*>("passwordCombo")->setCurrentIndex(5); // 0 = not present, 1 = first column
        const auto db = widget.buildDatabase();
        QVERIFY(db);
        const auto* entry = entryTitled(db, "remapped");
        QVERIFY(entry);
        QCOMPARE(entry->password(), QString("other pw"));
        QCOMPARE(entry->attributes()->value("Password *"), QString("plain"));
        QVERIFY2(widget.problems().join("\n").contains("exact text of \"Password\" was not used"),
                 qPrintable(widget.problems().join("\n")));
    }
}

// Catches: the new-database import (the wizard merges the imported database into the new one) losing entry ids, older
// versions, attachments, custom fields or the browser data of an entry. The same calls as DatabaseTabWidget makes.
void TestCsvImport::testImportIntoANewDatabaseKeepsEverything()
{
    auto source = QSharedPointer<Database>::create();
    auto* work = new Group();
    work->setName("Work");
    work->setParent(source->rootGroup());
    auto* rich = addRichEntry(work, "Rich entry");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto imported = exportAndImport(source, dir);
    QVERIFY(imported);

    auto newDb = QSharedPointer<Database>::create();
    Merger merger(imported.data(), newDb.data());
    merger.setSkipDatabaseCustomData(true);
    merger.merge();
    newDb->rootGroup()->setName(imported->rootGroup()->name());
    newDb->rootGroup()->setNotes(imported->rootGroup()->notes());

    const auto* found = newDb->rootGroup()->findEntryByUuid(rich->uuid());
    QVERIFY2(found, "the entry is missing from the new database");
    const auto problems = differences(rich, found);
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join("; ")));
}

// Catches: the copy that is made when the file's ids already exist in an open database (the file was exported from that
// database) losing anything but the ids, dates included. The same function as DatabaseTabWidget uses.
void TestCsvImport::testCopyWithNewIdsKeepsEverything()
{
    auto source = QSharedPointer<Database>::create();
    auto* work = new Group();
    work->setName("Work");
    work->setParent(source->rootGroup());
    auto* rich = addRichEntry(work, "Rich entry");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto imported = exportAndImport(source, dir);
    QVERIFY(imported);

    std::unique_ptr<Group> copy(imported->rootGroup()->cloneWithNewUuids());
    const auto copied = copy->entriesRecursive();
    QCOMPARE(copied.size(), 1);
    const auto* found = copied.first();
    QVERIFY2(found->uuid() != rich->uuid() && !found->uuid().isNull(), "the copy must have a new id");
    for (const auto* item : found->historyItems()) {
        QCOMPARE(item->uuid(), found->uuid());
    }
    // everything else is the same (the ids of the older versions follow the entry's new id)
    for (const auto& problem : differences(rich, found)) {
        QVERIFY2(problem == "entry id" || problem.endsWith(": entry id"), qPrintable("lost in the copy: " + problem));
    }
}

// Catches: a large attachment being written again for every older version of the entry (a file that grows with the
// number of edits), and the older versions losing it when it is left out
void TestCsvImport::testAnAttachmentIsNotRepeatedInEveryOlderVersion()
{
    QByteArray big;
    for (int i = 0; i < 100000; ++i) {
        big.append(static_cast<char>((i * 7 + i / 251) % 256));
    }
    auto source = QSharedPointer<Database>::create();
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setGroup(source->rootGroup());
    entry->setTitle("With attachment");
    entry->attachments()->set("big.bin", big);
    for (int edit = 0; edit < 3; ++edit) {
        entry->beginUpdate();
        entry->setNotes(QString("edit %1").arg(edit));
        entry->endUpdate();
    }
    QCOMPARE(entry->historyItems().size(), 3);

    CsvExporter exporter;
    const auto csv = exporter.exportDatabase(source);
    // base64 of the attachment is about 133,000 characters; it is written once, not four times
    QVERIFY2(csv.size() < 200000, qPrintable(QString("the file is %1 characters long").arg(csv.size())));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto imported = exportAndImport(source, dir);
    QVERIFY(imported);
    const auto* found = imported->rootGroup()->findEntryByUuid(entry->uuid());
    QVERIFY(found);
    const auto problems = differences(entry, found);
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join("; ")));
    QCOMPARE(found->historyItems().size(), 3);
    for (const auto* item : found->historyItems()) {
        QCOMPARE(item->attachments()->value("big.bin"), big);
    }
}

// Catches: data of an older version that cannot be understood being dropped without a word
void TestCsvImport::testOlderVersionDataThatCannotBeUnderstood()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CsvImportWidget widget;
    const QString history =
        "[{\"title\":\"t\",\"username\":\"\",\"password\":\"\",\"url\":\"\",\"notes\":\"\","
        "\"modified\":\"2018-05-06T07:08:09Z\",\"created\":\"nonsense\","
        "\"attachments\":{\"gone.bin\":null,\"bad.bin\":\"***\",\"fine.bin\":\"aGk=\"},"
        "\"icon\":9,\"tags\":\"old\",\"customData\":{\"k\":\"v\"},\"expires\":true,\"usageCount\":5}]";
    QVERIFY(loadText(widget,
                     dir,
                     csvLine({"Group", "Title", "Username", "Password", "History"})
                         + csvLine({"Web", "entry", "", "", history})));
    const auto db = widget.buildDatabase();
    QVERIFY(db);
    const auto* entry = entryTitled(db, "entry");
    QVERIFY(entry);
    QCOMPARE(entry->historyItems().size(), 1);
    const auto* item = entry->historyItems().first();
    QCOMPARE(item->attachments()->value("fine.bin"), QByteArray("hi"));
    QVERIFY(!item->attachments()->hasKey("gone.bin"));
    QCOMPARE(item->attributes()->value("Attachment bad.bin (CSV text that was not understood)"), QString("***"));
    QCOMPARE(item->iconNumber(), 9);
    QCOMPARE(item->tags(), QString("old"));
    QCOMPARE(item->customData()->value("k"), QString("v"));
    QVERIFY(item->timeInfo().expires());
    QCOMPARE(item->timeInfo().usageCount(), 5);

    const auto problems = widget.problems().join("\n");
    for (const QString& expected : QStringList{"attachment \"gone.bin\" is not in the entry",
                                               "attachment \"bad.bin\" is not valid base64",
                                               "\"nonsense\" is not a valid date (created)"}) {
        QVERIFY2(problems.contains(expected), qPrintable("not reported: " + expected + "\nreported:\n" + problems));
    }
}
