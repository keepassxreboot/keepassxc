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

#include "TestUrlOverride.h"

#include <QTest>

#include "core/Config.h"
#include "util/TemporaryFile.h"

QTEST_GUILESS_MAIN(TestUrlOverride)

void TestUrlOverride::initTestCase()
{
    QLocale::setDefault(QLocale::c());
}

void TestUrlOverride::testUrlOverrides()
{
    TemporaryFile tempFile;
    tempFile.open();
    tempFile.close();
    Config::createConfigFromFile(tempFile.fileName());

    // A freshly created config has never had anything saved for this feature, so a disabled
    // example rule is seeded to make the feature discoverable (see testDefaultSeedRule)
    QCOMPARE(UrlOverride::getRules().size(), 1);

    QList<UrlOverride::Rule> rules;
    rules.append({true, "ssh", "cmd://ssh {USERNAME}@{URL:HOST}"});
    rules.append({false, "vpn", "cmd://vpn-client --host {URL:HOST}"});
    rules.append({true, "ftp", "cmd://ftp {URL:HOST}"});
    UrlOverride::setRules(rules);

    // The rules must round-trip exactly, including order and the disabled rule
    auto storedRules = UrlOverride::getRules();
    QCOMPARE(storedRules.size(), 3);
    for (int i = 0; i < rules.size(); ++i) {
        QCOMPARE(storedRules[i].enabled, rules[i].enabled);
        QCOMPARE(storedRules[i].scheme, rules[i].scheme);
        QCOMPARE(storedRules[i].command, rules[i].command);
    }

    // Overwriting with a shorter list must not leave stale entries behind
    UrlOverride::setRules({{true, "ssh", "cmd://ssh {URL:HOST}"}});
    QCOMPARE(UrlOverride::getRules().size(), 1);

    // An explicitly saved empty list must stay empty (not be confused with "never configured")
    UrlOverride::setRules({});
    QVERIFY(UrlOverride::getRules().isEmpty());

    tempFile.remove();
}

void TestUrlOverride::testDefaultSeedRule()
{
    TemporaryFile tempFile;
    tempFile.open();
    tempFile.close();
    Config::createConfigFromFile(tempFile.fileName());

    // Fresh config: a disabled example rule is seeded so the feature is discoverable
    auto rules = UrlOverride::getRules();
    QCOMPARE(rules.size(), 1);
    QCOMPARE(rules.first().enabled, false);
    QCOMPARE(rules.first().scheme, QString("ssh"));
    QCOMPARE(rules.first().command, QString("cmd://ssh {USERNAME}@{URL:HOST}"));

    // It must never match anything on its own, since it's disabled by default
    QVERIFY(UrlOverride::findCommand("ssh://example.com").isEmpty());

    // Saving anything at all (including clearing the list) must permanently stop the reseed
    UrlOverride::setRules({});
    QVERIFY(UrlOverride::getRules().isEmpty());

    tempFile.remove();
}

void TestUrlOverride::testUrlOverrideSchemeNormalization()
{
    QCOMPARE(UrlOverride::normalizeScheme("ff://"), QString("ff"));
    QCOMPARE(UrlOverride::normalizeScheme("ff:"), QString("ff"));
    QCOMPARE(UrlOverride::normalizeScheme("ff"), QString("ff"));
    QCOMPARE(UrlOverride::normalizeScheme("  ff://  "), QString("ff"));

    // Extracted via the URI scheme grammar, not by stripping specific characters off either end:
    // stray leading junk, doubled-up separators, and trailing garbage that isn't ":"/"/" all still
    // resolve to just the scheme
    QCOMPARE(UrlOverride::normalizeScheme("/:ff://  "), QString("ff"));
    QCOMPARE(UrlOverride::normalizeScheme("://ff://  "), QString("ff"));
    QCOMPARE(UrlOverride::normalizeScheme("ff:&&  "), QString("ff"));

    // No letters at all: there is no scheme to extract
    QVERIFY(UrlOverride::normalizeScheme("://").isEmpty());
    QVERIFY(UrlOverride::normalizeScheme("123").isEmpty());
    QVERIFY(UrlOverride::normalizeScheme("").isEmpty());

    TemporaryFile tempFile;
    tempFile.open();
    tempFile.close();
    Config::createConfigFromFile(tempFile.fileName());

    // The scheme is not a regular expression: a scheme entered as "ff://" must be
    // normalized and stored as the literal scheme "ff"
    UrlOverride::setRules({{true, "ff://", "cmd://ff-handler {URL}"}});
    QCOMPARE(UrlOverride::getRules().first().scheme, QString("ff"));

    tempFile.remove();
}

void TestUrlOverride::testXmlSpecialCharactersRoundTrip()
{
    TemporaryFile tempFile;
    tempFile.open();
    tempFile.close();
    Config::createConfigFromFile(tempFile.fileName());

    // Rules are stored as hand-written XML; special characters in the command must survive a
    // save/load round-trip without breaking the XML structure or getting corrupted
    const QString command = R"(cmd://ssh -o ProxyCommand="nc %h %p & echo <ok> 'quoted'" {USERNAME}@{URL:HOST})";
    UrlOverride::setRules({{true, "ssh", command}});

    auto rules = UrlOverride::getRules();
    QCOMPARE(rules.size(), 1);
    QCOMPARE(rules.first().command, command);
    QCOMPARE(UrlOverride::findCommand("ssh://example.com"), command);

    tempFile.remove();
}

void TestUrlOverride::testFindUrlOverrideCommand()
{
    TemporaryFile tempFile;
    tempFile.open();
    tempFile.close();
    Config::createConfigFromFile(tempFile.fileName());

    QList<UrlOverride::Rule> rules;
    // Disabled rule must be skipped even though it matches
    rules.append({false, "ssh", "cmd://disabled-should-not-be-used"});
    rules.append({true, "ssh", "cmd://ssh {USERNAME}@{URL:HOST}"});
    rules.append({true, "VPN", "cmd://vpn-client --host {URL:HOST}"});
    rules.append({true, "ff", "cmd://msedge {URL:RMVSCM}"});
    UrlOverride::setRules(rules);

    // The scheme comparison is a literal, case-insensitive match (not a regular expression)
    QCOMPARE(UrlOverride::findCommand("ssh://user@example.com"), QString("cmd://ssh {USERNAME}@{URL:HOST}"));
    QCOMPARE(UrlOverride::findCommand("vpn://example.com"), QString("cmd://vpn-client --host {URL:HOST}"));
    QCOMPARE(UrlOverride::findCommand("ff://example.com"), QString("cmd://msedge {URL:RMVSCM}"));
    // "sshfs" must not match the "ssh" scheme rule
    QVERIFY(UrlOverride::findCommand("sshfs://example.com").isEmpty());
    QVERIFY(UrlOverride::findCommand("https://example.com").isEmpty());

    tempFile.remove();
}

void TestUrlOverride::testFindUrlOverrideCommandEdgeCases()
{
    TemporaryFile tempFile;
    tempFile.open();
    tempFile.close();
    Config::createConfigFromFile(tempFile.fileName());

    // With two enabled rules for the same scheme, the first one in the list must win
    UrlOverride::setRules({{true, "ssh", "cmd://first-ssh-handler"}, {true, "ssh", "cmd://second-ssh-handler"}});
    QCOMPARE(UrlOverride::findCommand("ssh://example.com"), QString("cmd://first-ssh-handler"));

    // An enabled rule with an empty scheme must never match anything
    UrlOverride::setRules({{true, "", "cmd://should-never-be-used"}});
    QVERIFY(UrlOverride::findCommand("ssh://example.com").isEmpty());
    QVERIFY(UrlOverride::findCommand("https://example.com").isEmpty());

    // An enabled rule with an empty command has nothing to run: it must be skipped rather than
    // matching and blocking a lower-priority rule for the same scheme
    UrlOverride::setRules({{true, "ssh", ""}, {true, "ssh", "cmd://real-ssh-handler"}});
    QCOMPARE(UrlOverride::findCommand("ssh://example.com"), QString("cmd://real-ssh-handler"));

    // If every rule for a scheme has an empty command, there is simply nothing to run
    UrlOverride::setRules({{true, "ssh", ""}});
    QVERIFY(UrlOverride::findCommand("ssh://example.com").isEmpty());

    // A URL without a scheme at all must not match any rule, even a permissive one
    UrlOverride::setRules({{true, "ssh", "cmd://should-never-be-used"}});
    QVERIFY(UrlOverride::findCommand("not-a-url-at-all").isEmpty());
    QVERIFY(UrlOverride::findCommand("").isEmpty());

    tempFile.remove();
}
