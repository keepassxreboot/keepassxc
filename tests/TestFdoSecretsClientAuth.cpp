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

#include "TestFdoSecretsClientAuth.h"

#include "mock/MockClock.h"

#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Merger.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "fdosecrets/ClientAuth.h"
#include "fdosecrets/dbus/DBusClient.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <utility>

QTEST_GUILESS_MAIN(TestFdoSecretsClientAuth)

using namespace FdoSecrets;

namespace
{
    ClientRecord sampleRecord()
    {
        ClientRecord record;
        record.id = QUuid::createUuid();
        record.name = QStringLiteral("firefox");
        record.created = Clock::currentDateTimeUtc();
        MatchRule rule;
        rule.conditions << RuleCondition{0, RuleCondition::Kind::Path, QStringLiteral("/usr/bin/firefox"), {}}
                        << RuleCondition{0,
                                         RuleCondition::Kind::Hash,
                                         QStringLiteral("aa00bb11cc22dd33"),
                                         QStringLiteral("sha256")};
        record.rules << rule;
        record.allEntries = AuthDecision::Undecided;
        return record;
    }

    // rewrite one top level json field of an otherwise valid record serialization
    QString tamper(const ClientRecord& record, const QString& field, const QJsonValue& value)
    {
        auto obj = QJsonDocument::fromJson(record.toJson().toUtf8()).object();
        obj.insert(field, value);
        return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }

    PeerInfo samplePeer()
    {
        return PeerInfo{QStringLiteral("local"),
                        1234,
                        true,
                        {
                            ProcInfo{1234,
                                     1000,
                                     QStringLiteral("/usr/bin/python3"),
                                     QStringLiteral("python3"),
                                     QStringLiteral("python3 myscript.py")},
                            ProcInfo{1000, 1, QStringLiteral("/usr/bin/zsh"), QStringLiteral("zsh"), {}},
                            ProcInfo{1, 0, QStringLiteral("/usr/lib/systemd/systemd"), {}, {}},
                        }};
    }

    // a client with a synthetic hierarchy and scripted executable hashes
    class FakeHashClient : public DBusClient
    {
    public:
        explicit FakeHashClient(PeerInfo info = samplePeer())
            : DBusClient(nullptr, std::move(info))
        {
        }

        QString exeHash(int depth, const QString& algo) override
        {
            return algo == DefaultExeHashAlgo ? hashes.value(depth) : QString();
        }

        QHash<int, QString> hashes;
    };

    RuleCondition path0(const QString& value = QStringLiteral("/usr/bin/python3"))
    {
        return {0, RuleCondition::Kind::Path, value, {}};
    }

    RuleCondition hashAt(int depth, const QString& value)
    {
        return {depth, RuleCondition::Kind::Hash, value, QStringLiteral("sha256")};
    }

    ClientRecord recordWithRule(const MatchRule& rule, AuthDecision allEntries = AuthDecision::Undecided)
    {
        ClientRecord record;
        record.id = QUuid::createUuid();
        record.name = QStringLiteral("test");
        record.created = Clock::currentDateTimeUtc();
        record.rules << rule;
        record.allEntries = allEntries;
        return record;
    }
} // namespace

void TestFdoSecretsClientAuth::initTestCase()
{
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());
}

void TestFdoSecretsClientAuth::init()
{
    m_clock = new MockClock(2026, 8, 19, 10, 30, 10);
    MockClock::setup(m_clock);
}

void TestFdoSecretsClientAuth::cleanup()
{
    MockClock::teardown();
    m_clock = nullptr;
}

void TestFdoSecretsClientAuth::testRecordJsonRoundTrip()
{
    auto record = sampleRecord();
    record.allEntries = AuthDecision::Allowed;

    MatchRule interpreterRule;
    interpreterRule.conditions << RuleCondition{1, RuleCondition::Kind::Name, QStringLiteral("myscript.py"), {}};
    record.rules << interpreterRule;

    const auto parsed = ClientRecord::fromJson(record.id, record.toJson());
    QVERIFY(parsed.isValid());
    QCOMPARE(parsed, record);

    // a record with no rules is valid: it can no longer match, but still anchors
    // per-entry decisions until the user removes it
    ClientRecord bare;
    bare.id = QUuid::createUuid();
    bare.created = Clock::currentDateTimeUtc();
    QCOMPARE(ClientRecord::fromJson(bare.id, bare.toJson()), bare);
}

void TestFdoSecretsClientAuth::testRecordJsonRejectsMalformed()
{
    const auto record = sampleRecord();
    const auto id = record.id;

    QVERIFY(!ClientRecord::fromJson(QUuid(), record.toJson()).isValid());
    QVERIFY(!ClientRecord::fromJson(id, QStringLiteral("not json")).isValid());
    QVERIFY(!ClientRecord::fromJson(id, QStringLiteral("[]")).isValid());
    QVERIFY(!ClientRecord::fromJson(id, tamper(record, "version", 2)).isValid());

    const auto condition = [&](const char* json) {
        return tamper(
            record, "rules", QJsonDocument::fromJson(QByteArray("[{\"conditions\":[") + json + "]}]").array());
    };
    // unknown kind must invalidate the record instead of being skipped:
    // skipping would broaden what the record matches
    QVERIFY(!ClientRecord::fromJson(id, condition(R"({"depth":0,"kind":"cgroup","value":"x"})")).isValid());
    QVERIFY(!ClientRecord::fromJson(id, condition(R"({"depth":-1,"kind":"path","value":"x"})")).isValid());
    QVERIFY(!ClientRecord::fromJson(id, condition(R"({"depth":0,"kind":"path","value":""})")).isValid());
    QVERIFY(!ClientRecord::fromJson(id, condition(R"({"depth":0,"kind":"hash","value":"aabb"})")).isValid());
    QVERIFY(!ClientRecord::fromJson(id, condition("")).isValid());
}

void TestFdoSecretsClientAuth::testRecordStoreRoundTrip()
{
    QScopedPointer<Database> db(new Database());

    QVERIFY(loadClientRecords(db.data()).isEmpty());

    auto record = sampleRecord();
    auto other = sampleRecord();
    other.name = QStringLiteral("secret-tool");
    saveClientRecord(db.data(), record);
    saveClientRecord(db.data(), other);

    auto records = loadClientRecords(db.data());
    QCOMPARE(records.size(), 2);
    QVERIFY(records.contains(record));
    QVERIFY(records.contains(other));
    QCOMPARE(loadClientRecord(db.data(), record.id), record);
    QVERIFY(!loadClientRecord(db.data(), QUuid::createUuid()).isValid());

    // update in place, e.g. after a fingerprint change
    record.rules[0].conditions[1].value = QStringLiteral("ff00ff00");
    saveClientRecord(db.data(), record);
    QCOMPARE(loadClientRecords(db.data()).size(), 2);
    QCOMPARE(loadClientRecord(db.data(), record.id), record);
}

void TestFdoSecretsClientAuth::testRemoveRecordSweepsEntries()
{
    QScopedPointer<Database> db(new Database());
    auto record = sampleRecord();
    auto other = sampleRecord();
    saveClientRecord(db.data(), record);
    saveClientRecord(db.data(), other);

    auto entry = new Entry();
    entry->setGroup(db->rootGroup());
    auto subGroup = new Group();
    subGroup->setParent(db->rootGroup());
    auto nested = new Entry();
    nested->setGroup(subGroup);

    setEntryClientDecision(entry, record.id, AuthDecision::Allowed);
    setEntryClientDecision(entry, other.id, AuthDecision::Denied);
    setEntryClientDecision(nested, record.id, AuthDecision::Allowed);

    removeClientRecord(db.data(), record.id);

    QVERIFY(!loadClientRecord(db.data(), record.id).isValid());
    QCOMPARE(loadClientRecords(db.data()).size(), 1);
    QVERIFY(!entryClientDecisions(entry).contains(record.id));
    QCOMPARE(entryClientDecisions(entry).value(other.id), AuthDecision::Denied);
    QVERIFY(entryClientDecisions(nested).isEmpty());
}

void TestFdoSecretsClientAuth::testEntryClientDecisions()
{
    QScopedPointer<Database> db(new Database());
    auto entry = new Entry();
    entry->setGroup(db->rootGroup());

    const auto idA = QUuid::createUuid();
    const auto idB = QUuid::createUuid();

    QVERIFY(entryClientDecisions(entry).isEmpty());

    setEntryClientDecision(entry, idA, AuthDecision::Allowed);
    setEntryClientDecision(entry, idB, AuthDecision::Denied);
    auto decisions = entryClientDecisions(entry);
    QCOMPARE(decisions.size(), 2);
    QCOMPARE(decisions.value(idA), AuthDecision::Allowed);
    QCOMPARE(decisions.value(idB), AuthDecision::Denied);

    // only the persistent decisions are representable; Undecided removes
    setEntryClientDecision(entry, idA, AuthDecision::Undecided);
    QVERIFY(!entryClientDecisions(entry).contains(idA));

    // the customData key disappears with the last decision
    setEntryClientDecision(entry, idB, AuthDecision::Undecided);
    QVERIFY(entryClientDecisions(entry).isEmpty());
    QVERIFY(!entry->customData()->hasKey(QStringLiteral("FDO_SECRETS_AUTH")));

    // garbage in the entry customData is ignored, not crashed on
    entry->customData()->set(QStringLiteral("FDO_SECRETS_AUTH"), QStringLiteral("not json"));
    QVERIFY(entryClientDecisions(entry).isEmpty());
}

void TestFdoSecretsClientAuth::testRecordKeyProtected()
{
    QScopedPointer<Database> db(new Database());
    const auto record = sampleRecord();
    saveClientRecord(db.data(), record);
    QVERIFY(db->metadata()->customData()->isProtected(record.customDataKey()));
}

void TestFdoSecretsClientAuth::testMergeKeepsRecords()
{
    QScopedPointer<Database> dbDestination(new Database());
    QScopedPointer<Database> dbSource(new Database());

    m_clock->advanceSecond(1);
    const auto record = sampleRecord();
    saveClientRecord(dbDestination.data(), record);
    dbDestination->metadata()->customData()->set(QStringLiteral("unprotected"), QStringLiteral("value"));

    // make the source's customData newer: the merge then deletes destination-only
    // keys wholesale, except protected ones
    m_clock->advanceSecond(1);
    dbSource->metadata()->customData()->set(QStringLiteral("sourceKey"), QStringLiteral("value"));

    m_clock->advanceSecond(1);
    Merger merger(dbSource.data(), dbDestination.data());
    merger.merge();

    QVERIFY(!dbDestination->metadata()->customData()->hasKey(QStringLiteral("unprotected")));
    QCOMPARE(loadClientRecord(dbDestination.data(), record.id), record);
}

void TestFdoSecretsClientAuth::testExeHash()
{
#ifndef Q_OS_LINUX
    QSKIP("/proc/PID/exe hashing is only implemented on Linux");
#endif
    const auto ownPid = static_cast<uint>(QCoreApplication::applicationPid());
    PeerInfo info{QStringLiteral("local"), ownPid, true, {ProcInfo{ownPid, 0, {}, {}, {}}, ProcInfo{0, 0, {}, {}, {}}}};
    DBusClient client(nullptr, info);

    // hashing our own pid via /proc/PID/exe must match hashing the binary by path
    QFile self(QCoreApplication::applicationFilePath());
    QVERIFY(self.open(QIODevice::ReadOnly));
    QCryptographicHash expected(QCryptographicHash::Sha256);
    QVERIFY(expected.addData(&self));

    const auto hash = client.exeHash(0, DefaultExeHashAlgo);
    QCOMPARE(hash, QString::fromLatin1(expected.result().toHex()));
    QCOMPARE(client.exeHash(0, DefaultExeHashAlgo), hash);

    // an unknown algorithm fails closed instead of falling back to another one
    QCOMPARE(client.exeHash(0, QStringLiteral("md5")), QString());

    // pid 0 has no /proc entry: fails closed, and the failure is cached too
    QCOMPARE(client.exeHash(1, DefaultExeHashAlgo), QString());
    QCOMPARE(client.exeHash(1, DefaultExeHashAlgo), QString());

    // outside the hierarchy
    QCOMPARE(client.exeHash(-1, DefaultExeHashAlgo), QString());
    QCOMPARE(client.exeHash(2, DefaultExeHashAlgo), QString());
}

void TestFdoSecretsClientAuth::testRuleMatching()
{
    FakeHashClient client;
    client.hashes.insert(0, QStringLiteral("hash0"));
    client.hashes.insert(1, QStringLiteral("hash1"));

    // single conditions on the calling process
    QVERIFY(recordWithRule({{path0()}}).matches(client));
    QVERIFY(!recordWithRule({{path0(QStringLiteral("/usr/bin/python"))}}).matches(client));
    QVERIFY(recordWithRule({{{0, RuleCondition::Kind::Name, QStringLiteral("python3"), {}}}}).matches(client));
    QVERIFY(
        !recordWithRule({{{0, RuleCondition::Kind::Name, QStringLiteral("/usr/bin/python3"), {}}}}).matches(client));
    QVERIFY(recordWithRule({{hashAt(0, QStringLiteral("hash0"))}}).matches(client));
    QVERIFY(!recordWithRule({{hashAt(0, QStringLiteral("other"))}}).matches(client));

    // ancestors by depth
    QVERIFY(recordWithRule({{{1, RuleCondition::Kind::Path, QStringLiteral("/usr/bin/zsh"), {}}}}).matches(client));
    QVERIFY(!recordWithRule({{{5, RuleCondition::Kind::Path, QStringLiteral("/usr/bin/zsh"), {}}}}).matches(client));

    // conditions are a conjunction
    QVERIFY(recordWithRule({{path0(), hashAt(0, QStringLiteral("hash0")), hashAt(1, QStringLiteral("hash1"))}})
                .matches(client));
    QVERIFY(!recordWithRule({{path0(), {1, RuleCondition::Kind::Path, QStringLiteral("/usr/bin/bash"), {}}}})
                 .matches(client));

    // rules are a disjunction
    auto record = recordWithRule({{path0(QStringLiteral("/usr/bin/kitty"))}});
    record.rules << MatchRule{{path0()}};
    QVERIFY(record.matches(client));

    // fail closed on anything not understood or unavailable
    auto unknownAlgo = hashAt(0, QStringLiteral("hash0"));
    unknownAlgo.algo = QStringLiteral("md5");
    QVERIFY(!recordWithRule({{unknownAlgo}}).matches(client));
    QVERIFY(!recordWithRule({{hashAt(2, QStringLiteral("anything"))}}).matches(client)); // no hash available
    QVERIFY(!recordWithRule(MatchRule{}).matches(client)); // empty rule must not match everything
    QVERIFY(!ClientRecord{}.matches(client)); // nor a record without rules
}

void TestFdoSecretsClientAuth::testBuildMatchRule()
{
    FakeHashClient client;
    client.hashes.insert(0, QStringLiteral("hash0"));

    // path and content when both are available; path only when the content
    // cannot be hashed
    QCOMPARE(buildMatchRule(client, {0, 1}).conditions,
             (QList<RuleCondition>{{0, RuleCondition::Kind::Path, QStringLiteral("/usr/bin/python3"), {}},
                                   hashAt(0, QStringLiteral("hash0")),
                                   {1, RuleCondition::Kind::Path, QStringLiteral("/usr/bin/zsh"), {}}}));

    // content only when the path is unknown, e.g. a deleted binary that is
    // still hashable through /proc/PID/exe
    auto peer = samplePeer();
    peer.hierarchy[0].exePath.clear();
    FakeHashClient pathless(peer);
    pathless.hashes.insert(0, QStringLiteral("hash0"));
    QCOMPARE(buildMatchRule(pathless, {0}).conditions, (QList<RuleCondition>{hashAt(0, QStringLiteral("hash0"))}));

    // neither: the depth contributes nothing and an all-unusable rule is empty
    FakeHashClient bare(peer);
    QVERIFY(buildMatchRule(bare, {0}).conditions.isEmpty());
    QVERIFY(buildMatchRule(client, {7}).conditions.isEmpty());
}

void TestFdoSecretsClientAuth::testFingerprintChanged()
{
    FakeHashClient client;
    client.hashes.insert(0, QStringLiteral("hash0"));
    client.hashes.insert(1, QStringLiteral("hash1"));

    // identified by path, hash no longer matches
    QSet<int> depths;
    auto record = recordWithRule({{path0(), hashAt(0, QStringLiteral("stale"))}});
    QVERIFY(!record.matches(client));
    QVERIFY(record.fingerprintChanged(client, &depths));
    QCOMPARE(depths, QSet<int>{0});

    // several hierarchy levels changed at once are all reported
    depths.clear();
    record = recordWithRule({{path0(), hashAt(0, QStringLiteral("stale")), hashAt(1, QStringLiteral("stale"))}});
    QVERIFY(record.fingerprintChanged(client, &depths));
    QCOMPARE(depths, (QSet<int>{0, 1}));

    // a different client entirely is not a fingerprint change
    record = recordWithRule({{path0(QStringLiteral("/usr/bin/kitty")), hashAt(0, QStringLiteral("stale"))}});
    QVERIFY(!record.fingerprintChanged(client));

    // nor is a record that still fully matches through another rule
    record = recordWithRule({{path0(), hashAt(0, QStringLiteral("stale"))}});
    record.rules << MatchRule{{path0()}};
    QVERIFY(record.matches(client));
    QVERIFY(!record.fingerprintChanged(client));
}

void TestFdoSecretsClientAuth::testResolveOverlap()
{
    QScopedPointer<Database> db(new Database());
    FakeHashClient client;

    // no records at all
    QVERIFY(!resolveClient(db.data(), client).record.isValid());

    auto allowing = recordWithRule({{path0()}}, AuthDecision::Allowed);
    m_clock->advanceSecond(1);
    auto denying = recordWithRule({{path0()}}, AuthDecision::Denied);
    m_clock->advanceSecond(1);
    saveClientRecord(db.data(), allowing);
    saveClientRecord(db.data(), denying);

    // overlapping records: the denying catch-all wins even though it is younger
    QCOMPARE(resolveClient(db.data(), client).record, denying);

    // among equals the earliest created wins
    m_clock->advanceSecond(1);
    removeClientRecord(db.data(), denying.id);
    auto younger = recordWithRule({{path0()}}, AuthDecision::Allowed);
    saveClientRecord(db.data(), younger);
    QCOMPARE(resolveClient(db.data(), client).record, allowing);

    // removed records no longer resolve
    m_clock->advanceSecond(1);
    removeClientRecord(db.data(), allowing.id);
    removeClientRecord(db.data(), younger.id);
    QVERIFY(!resolveClient(db.data(), client).record.isValid());

    // a fingerprint change surfaces only when nothing matches outright
    m_clock->advanceSecond(1);
    auto changed = recordWithRule({{path0(), hashAt(0, QStringLiteral("stale"))}});
    saveClientRecord(db.data(), changed);
    auto res = resolveClient(db.data(), client);
    QVERIFY(!res.record.isValid());
    QVERIFY(res.fingerprintChanged);
    QCOMPARE(res.changed, changed);
    QCOMPARE(res.mismatchedDepths, QSet<int>{0});

    m_clock->advanceSecond(1);
    saveClientRecord(db.data(), recordWithRule({{path0()}}, AuthDecision::Allowed));
    res = resolveClient(db.data(), client);
    QVERIFY(res.record.isValid());
    QVERIFY(!res.fingerprintChanged);
}

void TestFdoSecretsClientAuth::testResolverDecision()
{
    QScopedPointer<Database> db(new Database());
    auto entry = new Entry();
    entry->setGroup(db->rootGroup());
    FakeHashClient client;

    // unknown client
    QCOMPARE(persistedDecision(entry, client), AuthDecision::Undecided);

    // record without any decision for this entry
    auto record = recordWithRule({{path0()}});
    saveClientRecord(db.data(), record);
    QCOMPARE(persistedDecision(entry, client), AuthDecision::Undecided);

    // catch-all applies to entries without their own decision
    m_clock->advanceSecond(1);
    record.allEntries = AuthDecision::Allowed;
    saveClientRecord(db.data(), record);
    QCOMPARE(persistedDecision(entry, client), AuthDecision::Allowed);

    // the entry's own decision overrides the catch-all
    setEntryClientDecision(entry, record.id, AuthDecision::Denied);
    QCOMPARE(persistedDecision(entry, client), AuthDecision::Denied);

    // decisions referencing other records are not consulted
    setEntryClientDecision(entry, record.id, AuthDecision::Undecided);
    setEntryClientDecision(entry, QUuid::createUuid(), AuthDecision::Denied);
    QCOMPARE(persistedDecision(entry, client), AuthDecision::Allowed);
}
