/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "TestAutoType.h"

#include <QPluginLoader>
#include <QTest>

#include "autotype/AutoType.h"
#include "autotype/AutoTypePlatformPlugin.h"
#include "autotype/test/AutoTypeTestInterface.h"
#include "core/Config.h"
#include "core/Group.h"
#include "core/Resources.h"
#include "crypto/Crypto.h"
#include "gui/MessageBox.h"
#include "gui/osutils/OSUtils.h"

QTEST_GUILESS_MAIN(TestAutoType)

void TestAutoType::initTestCase()
{
    QVERIFY(Crypto::init());
    Config::createTempFileInstance();
    config()->set(Config::AutoTypeDelay, 1);
    config()->set(Config::Security_AutoTypeAsk, false);
    AutoType::createTestInstance();

    QPluginLoader loader(resources()->pluginPath("keepassxc-autotype-test"));
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    QVERIFY(loader.instance());

    m_platform = qobject_cast<AutoTypePlatformInterface*>(loader.instance());
    QVERIFY(m_platform);

    m_test = qobject_cast<AutoTypeTestInterface*>(loader.instance());
    QVERIFY(m_test);

    m_autoType = AutoType::instance();
}

void TestAutoType::init()
{
    config()->set(Config::AutoTypeEntryTitleMatch, false);
    m_test->clearActions();

    m_db = QSharedPointer<Database>::create();
    m_dbList.clear();
    m_dbList.append(m_db);
    m_group = m_db->rootGroup();

    AutoTypeAssociations::Association association;

    m_entry1 = new Entry();
    m_entry1->setGroup(m_group);
    m_entry1->setUsername("myuser");
    m_entry1->setPassword("mypass");
    association.window = "custom window";
    association.sequence = "{username}association{password}";
    m_entry1->autoTypeAssociations()->add(association);

    m_entry2 = new Entry();
    m_entry2->setGroup(m_group);
    m_entry2->setPassword("myuser");
    m_entry2->setTitle("entry title");

    m_entry3 = new Entry();
    m_entry3->setGroup(m_group);
    m_entry3->setPassword("regex");
    association.window = "//REGEX1//";
    association.sequence = "regex1";
    m_entry3->autoTypeAssociations()->add(association);
    association.window = "//^REGEX2$//";
    association.sequence = "regex2";
    m_entry3->autoTypeAssociations()->add(association);
    association.window = "//^REGEX3-([rd]\\d){2}$//";
    association.sequence = "regex3";
    m_entry3->autoTypeAssociations()->add(association);

    m_entry4 = new Entry();
    m_entry4->setGroup(m_group);
    m_entry4->setPassword("custom_attr");
    m_entry4->attributes()->set("CUSTOM", "Attribute", false);
    m_entry4->attributes()->set("CustomAttrFirst", "AttrValueFirst", false);
    m_entry4->attributes()->set("CustomAttrSecond", "AttrValueSecond", false);
    m_entry4->attributes()->set("CustomAttrThird", "AttrValueThird", false);
    association.window = "//^CustomAttr1$//";
    association.sequence = "{PASSWORD}:{S:CUSTOM}";
    m_entry4->autoTypeAssociations()->add(association);
    association.window = "//^CustomAttr2$//";
    association.sequence = "{S:CuStOm}";
    m_entry4->autoTypeAssociations()->add(association);
    association.window = "//^CustomAttr3$//";
    association.sequence = "{PaSSworD}";
    m_entry4->autoTypeAssociations()->add(association);
    association.window = "//^{S:CustomAttrFirst}$//";
    association.sequence = "custom_attr_first";
    m_entry4->autoTypeAssociations()->add(association);
    association.window = "//{S:CustomAttrFirst}And{S:CustomAttrSecond}//";
    association.sequence = "custom_attr_first_and_second";
    m_entry4->autoTypeAssociations()->add(association);
    association.window = "//{S:CustomAttrThird}//";
    association.sequence = "custom_attr_third";
    m_entry4->autoTypeAssociations()->add(association);

    m_entry5 = new Entry();
    m_entry5->setGroup(m_group);
    m_entry5->setPassword("example5");
    m_entry5->setTitle("some title");
    m_entry5->setUrl("http://example.org");
}

void TestAutoType::cleanup()
{
}

void TestAutoType::testInternal()
{
    QVERIFY(m_platform->activeWindowTitle().isEmpty());

    m_test->setActiveWindowTitle("Test");
    QCOMPARE(m_platform->activeWindowTitle(), QString("Test"));
}

void TestAutoType::testSingleAutoType()
{
    m_autoType->performAutoType(m_entry1);

    QCOMPARE(m_test->actionCount(), 14);
    QCOMPARE(m_test->actionChars(),
             QString("myuser%1mypass%2").arg(m_test->keyToString(Qt::Key_Tab)).arg(m_test->keyToString(Qt::Key_Enter)));
}

void TestAutoType::testGlobalAutoTypeWithNoMatch()
{
    m_test->setActiveWindowTitle("nomatch");
    MessageBox::setNextAnswer(MessageBox::Ok);
    m_autoType->performGlobalAutoType(m_dbList);

    QCOMPARE(m_test->actionChars(), QString());
}

void TestAutoType::testGlobalAutoTypeWithOneMatch()
{
    m_test->setActiveWindowTitle("custom window");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);

    QCOMPARE(m_test->actionChars(), QString("%1association%2").arg(m_entry1->username()).arg(m_entry1->password()));
}

void TestAutoType::testGlobalAutoTypeTitleMatch()
{
    config()->set(Config::AutoTypeEntryTitleMatch, true);

    m_test->setActiveWindowTitle("An Entry Title!");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);

    QCOMPARE(m_test->actionChars(), QString("%1%2").arg(m_entry2->password(), m_test->keyToString(Qt::Key_Enter)));
}

void TestAutoType::testGlobalAutoTypeUrlMatch()
{
    config()->set(Config::AutoTypeEntryTitleMatch, true);

    m_test->setActiveWindowTitle("Dummy - http://example.org/ - <My Browser>");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);

    QCOMPARE(m_test->actionChars(), QString("%1%2").arg(m_entry5->password(), m_test->keyToString(Qt::Key_Enter)));
}

void TestAutoType::testGlobalAutoTypeUrlSubdomainMatch()
{
    config()->set(Config::AutoTypeEntryTitleMatch, true);

    m_test->setActiveWindowTitle("Dummy - http://sub.example.org/ - <My Browser>");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);

    QCOMPARE(m_test->actionChars(), QString("%1%2").arg(m_entry5->password(), m_test->keyToString(Qt::Key_Enter)));
}

void TestAutoType::testGlobalAutoTypeTitleMatchDisabled()
{
    m_test->setActiveWindowTitle("An Entry Title!");
    emit osUtils->globalShortcutTriggered("autotype");
    MessageBox::setNextAnswer(MessageBox::Ok);
    m_autoType->performGlobalAutoType(m_dbList);

    QCOMPARE(m_test->actionChars(), QString());
}

void TestAutoType::testGlobalAutoTypeRegExp()
{
    // substring matches are ok
    m_test->setActiveWindowTitle("lorem REGEX1 ipsum");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);
    QCOMPARE(m_test->actionChars(), QString("regex1"));
    m_test->clearActions();

    // should be case-insensitive
    m_test->setActiveWindowTitle("lorem regex1 ipsum");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);
    QCOMPARE(m_test->actionChars(), QString("regex1"));
    m_test->clearActions();

    // exact match
    m_test->setActiveWindowTitle("REGEX2");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);
    QCOMPARE(m_test->actionChars(), QString("regex2"));
    m_test->clearActions();

    // a bit more complicated regex
    m_test->setActiveWindowTitle("REGEX3-R2D2");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);
    QCOMPARE(m_test->actionChars(), QString("regex3"));
    m_test->clearActions();

    // with custom attributes
    m_test->setActiveWindowTitle("CustomAttr1");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);
    QCOMPARE(m_test->actionChars(), QString("custom_attr:Attribute"));
    m_test->clearActions();

    // with (non uppercase) undefined custom attributes
    m_test->setActiveWindowTitle("CustomAttr2");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);
    QCOMPARE(m_test->actionChars(), QString(""));
    m_test->clearActions();

    // with mixedcase default attributes
    m_test->setActiveWindowTitle("CustomAttr3");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);
    QCOMPARE(m_test->actionChars(), QString("custom_attr"));
    m_test->clearActions();

    // with resolve placeholders in window association title
    m_test->setActiveWindowTitle("AttrValueFirst");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);
    QCOMPARE(m_test->actionChars(), QString("custom_attr_first"));
    m_test->clearActions();

    m_test->setActiveWindowTitle("lorem AttrValueFirstAndAttrValueSecond ipsum");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);
    QCOMPARE(m_test->actionChars(), QString("custom_attr_first_and_second"));
    m_test->clearActions();

    m_test->setActiveWindowTitle("lorem AttrValueThird ipsum");
    emit osUtils->globalShortcutTriggered("autotype");
    m_autoType->performGlobalAutoType(m_dbList);
    QCOMPARE(m_test->actionChars(), QString("custom_attr_third"));
    m_test->clearActions();
}

void TestAutoType::testAutoTypeResults()
{
    QScopedPointer<Entry> entry(new Entry());
    entry->setUsername("Username");
    entry->setPassword("Password@1");
    entry->setUrl("https://example.com");
    entry->attributes()->set("attr1", "value1");
    entry->attributes()->set("attr2", "decode%20me");

    QFETCH(QString, sequence);
    QFETCH(QString, expectedResult);

    m_autoType->performAutoTypeWithSequence(entry.data(), sequence);
    QCOMPARE(m_test->actionChars(), expectedResult);
}

void TestAutoType::testAutoTypeResults_data()
{
    QTest::addColumn<QString>("sequence");
    QTest::addColumn<QString>("expectedResult");

    // Normal Sequences
    QTest::newRow("Sequence with Attributes") << QString("{USERNAME} {PASSWORD} {URL} {S:attr1}")
                                              << QString("Username Password@1 https://example.com value1");
    QTest::newRow("Sequence with Comment") << QString("{USERNAME}{TAB}{C:Extra Tab}{TAB}{S:attr1}")
                                           << QString("Username[Key0x1000001][Key0x1000001]value1");

    // Conversions and Replacements
    QTest::newRow("T-CONV UPPER") << QString("{T-CONV:/{USERNAME}/UPPER/}") << QString("USERNAME");
    QTest::newRow("T-CONV LOWER") << QString("{T-CONV:/{USERNAME}/LOWER/}") << QString("username");
    QTest::newRow("T-CONV BASE64") << QString("{T-CONV:/{USERNAME}/BASE64/}") << QString("VXNlcm5hbWU=");
    QTest::newRow("T-CONV HEX") << QString("{T-CONV:/{USERNAME}/HEX/}") << QString("557365726e616d65");
    QTest::newRow("T-CONV URI ENCODE") << QString("{T-CONV:/{URL}/URI/}") << QString("https%3A%2F%2Fexample.com");
    QTest::newRow("T-CONV URI DECODE") << QString("{T-CONV:/{S:attr2}/URI-DEC/}") << QString("decode me");
    QTest::newRow("T-REPLACE-RX") << QString("{T-REPLACE-RX:/{USERNAME}/(User)/$1Pass/}") << QString("UserPassname");
}

void TestAutoType::testAutoTypeSyntaxChecks()
{
    auto entry = new Entry();
    QString error;

    // Huge sequence
    QVERIFY2(AutoType::verifyAutoTypeSyntax(
                 "{F1 23}{~ 23}{% 23}{^}{F12}{(}{) 23}{[}{[}{]}{Delay=23}{+}{SUBTRACT}~+%@fixedstring", entry, error),
             error.toLatin1());

    QVERIFY2(AutoType::verifyAutoTypeSyntax("{NUMPAD1 3}", entry, error), error.toLatin1());

    QVERIFY2(AutoType::verifyAutoTypeSyntax("{S:SPECIALTOKEN}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{S:SPECIAL TOKEN}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{S:SPECIAL_TOKEN}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{S:SPECIAL-TOKEN}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{S:SPECIAL:TOKEN}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{S:SPECIAL_TOKEN}{ENTER}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{S:FOO}{S:HELLO WORLD}", entry, error), error.toLatin1());
    QVERIFY2(!AutoType::verifyAutoTypeSyntax("{S:SPECIAL_TOKEN{}}", entry, error), error.toLatin1());

    QVERIFY2(AutoType::verifyAutoTypeSyntax("{BEEP 3 3}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{BEEP 3}", entry, error), error.toLatin1());

    QVERIFY2(AutoType::verifyAutoTypeSyntax("{VKEY 0x01}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{VKEY VK_LBUTTON}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{VKEY-EX 0x01}", entry, error), error.toLatin1());
    // Bad sequence
    QVERIFY2(!AutoType::verifyAutoTypeSyntax("{{{}}{}{}}{{}}", entry, error), error.toLatin1());
    // Good sequence
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{{}{}}{}}{{}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{]}{[}{[}{]}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{)}{(}{(}{)}", entry, error), error.toLatin1());
    // High delay
    QVERIFY2(!AutoType::verifyAutoTypeSyntax("{DELAY 50000}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{delay 50}", entry, error), error.toLatin1());
    // Slow typing
    QVERIFY2(!AutoType::verifyAutoTypeSyntax("{DELAY=50000}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{delay=50}", entry, error), error.toLatin1());
    // Many repetition
    QVERIFY2(!AutoType::verifyAutoTypeSyntax("{LEFT 50000000}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{SPACE 10}{TAB 3}{RIGHT 50}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{delay 5000000000}", entry, error), error.toLatin1());
    // Conversion and Regex
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{T-CONV:/{USERNAME}/base64/}", entry, error), error.toLatin1());
    QVERIFY2(!AutoType::verifyAutoTypeSyntax("{T-CONV:/{USERNAME}/junk/}", entry, error), error.toLatin1());
    QVERIFY2(!AutoType::verifyAutoTypeSyntax("{T-CONV:}", entry, error), error.toLatin1());
    QVERIFY2(AutoType::verifyAutoTypeSyntax("{T-REPLACE-RX:/{USERNAME}/a/b/}", entry, error), error.toLatin1());
    QVERIFY2(!AutoType::verifyAutoTypeSyntax("{T-REPLACE-RX:/{USERNAME}/a/}", entry, error), error.toLatin1());
    QVERIFY2(!AutoType::verifyAutoTypeSyntax("{T-REPLACE-RX:}", entry, error), error.toLatin1());
}

void TestAutoType::testAutoTypeEffectiveSequences()
{
    QString defaultSequence("{USERNAME}{TAB}{PASSWORD}{ENTER}");
    QString sequenceG1("{TEST_GROUP1}");
    QString sequenceG3("{TEST_GROUP3}");
    QString sequenceE2("{TEST_ENTRY2}");
    QString sequenceDisabled("{TEST_DISABLED}");
    QString sequenceOrphan("{TEST_ORPHAN}");

    QScopedPointer<Database> db(new Database());
    QPointer<Group> rootGroup = db->rootGroup();

    // Group with autotype enabled and custom default sequence
    QPointer<Group> group1 = new Group();
    group1->setParent(rootGroup);
    group1->setDefaultAutoTypeSequence(sequenceG1);

    // Child group with inherit
    QPointer<Group> group2 = new Group();
    group2->setParent(group1);

    // Group with autotype disabled and custom default sequence
    QPointer<Group> group3 = new Group();
    group3->setParent(group1);
    group3->setAutoTypeEnabled(Group::Disable);
    group3->setDefaultAutoTypeSequence(sequenceG3);

    QCOMPARE(rootGroup->defaultAutoTypeSequence(), QString());
    QCOMPARE(rootGroup->effectiveAutoTypeSequence(), defaultSequence);
    QCOMPARE(group1->defaultAutoTypeSequence(), sequenceG1);
    QCOMPARE(group1->effectiveAutoTypeSequence(), sequenceG1);
    QCOMPARE(group2->defaultAutoTypeSequence(), QString());
    QCOMPARE(group2->effectiveAutoTypeSequence(), sequenceG1);
    QCOMPARE(group3->defaultAutoTypeSequence(), sequenceG3);
    QCOMPARE(group3->effectiveAutoTypeSequence(), QString());

    // Entry from root group
    QPointer<Entry> entry1 = new Entry();
    entry1->setGroup(rootGroup);

    // Entry with custom default sequence
    QPointer<Entry> entry2 = new Entry();
    entry2->setDefaultAutoTypeSequence(sequenceE2);
    entry2->setGroup(rootGroup);

    // Entry from enabled child group
    QPointer<Entry> entry3 = new Entry();
    entry3->setGroup(group2);

    // Entry from disabled group
    QPointer<Entry> entry4 = new Entry();
    entry4->setDefaultAutoTypeSequence(sequenceDisabled);
    entry4->setGroup(group3);

    // Entry from enabled group with disabled autotype
    QPointer<Entry> entry5 = new Entry();
    entry5->setGroup(group2);
    entry5->setDefaultAutoTypeSequence(sequenceDisabled);
    entry5->setAutoTypeEnabled(false);

    // Entry with no parent
    QScopedPointer<Entry> entry6(new Entry());
    entry6->setDefaultAutoTypeSequence(sequenceOrphan);

    QCOMPARE(entry1->defaultAutoTypeSequence(), QString());
    QCOMPARE(entry1->effectiveAutoTypeSequence(), defaultSequence);
    QCOMPARE(entry2->defaultAutoTypeSequence(), sequenceE2);
    QCOMPARE(entry2->effectiveAutoTypeSequence(), sequenceE2);
    QCOMPARE(entry3->defaultAutoTypeSequence(), QString());
    QCOMPARE(entry3->effectiveAutoTypeSequence(), sequenceG1);
    QCOMPARE(entry4->defaultAutoTypeSequence(), sequenceDisabled);
    QCOMPARE(entry4->effectiveAutoTypeSequence(), QString());
    QCOMPARE(entry5->defaultAutoTypeSequence(), sequenceDisabled);
    QCOMPARE(entry5->effectiveAutoTypeSequence(), QString());
    QCOMPARE(entry6->defaultAutoTypeSequence(), sequenceOrphan);
    QCOMPARE(entry6->effectiveAutoTypeSequence(), QString());
}
