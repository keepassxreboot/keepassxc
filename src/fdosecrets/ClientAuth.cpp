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

#include "ClientAuth.h"

#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "fdosecrets/dbus/DBusClient.h"

#include "core/Clock.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <algorithm>

namespace FdoSecrets
{
    const QString DefaultExeHashAlgo = QStringLiteral("sha256");

    namespace
    {
        constexpr auto RecordVersion = 1;
        constexpr auto EntryAuthVersion = 1;
        // key in the entry's own customData holding the per-entry decisions
        const QString EntryAuthKey = QStringLiteral("FDO_SECRETS_AUTH");

        QString decisionToString(AuthDecision decision)
        {
            switch (decision) {
            case AuthDecision::Allowed:
                return QStringLiteral("allow");
            case AuthDecision::Denied:
                return QStringLiteral("deny");
            default:
                return {};
            }
        }

        AuthDecision decisionFromString(const QString& str)
        {
            if (str == QStringLiteral("allow")) {
                return AuthDecision::Allowed;
            }
            if (str == QStringLiteral("deny")) {
                return AuthDecision::Denied;
            }
            return AuthDecision::Undecided;
        }

        QString kindToString(RuleCondition::Kind kind)
        {
            switch (kind) {
            case RuleCondition::Kind::Path:
                return QStringLiteral("path");
            case RuleCondition::Kind::Name:
                return QStringLiteral("name");
            case RuleCondition::Kind::Hash:
                return QStringLiteral("hash");
            }
            return {};
        }

        bool kindFromString(const QString& str, RuleCondition::Kind& kind)
        {
            if (str == QStringLiteral("path")) {
                kind = RuleCondition::Kind::Path;
            } else if (str == QStringLiteral("name")) {
                kind = RuleCondition::Kind::Name;
            } else if (str == QStringLiteral("hash")) {
                kind = RuleCondition::Kind::Hash;
            } else {
                return false;
            }
            return true;
        }
    } // namespace

    bool RuleCondition::operator==(const RuleCondition& other) const
    {
        return depth == other.depth && kind == other.kind && value == other.value && algo == other.algo;
    }

    bool RuleCondition::operator!=(const RuleCondition& other) const
    {
        return !(*this == other);
    }

    bool MatchRule::operator==(const MatchRule& other) const
    {
        return conditions == other.conditions;
    }

    bool MatchRule::operator!=(const MatchRule& other) const
    {
        return !(*this == other);
    }

    bool RuleCondition::matches(DBusClient& client) const
    {
        const auto& hierarchy = client.processInfo().hierarchy;
        if (depth >= hierarchy.size() || value.isEmpty()) {
            return false;
        }
        const auto& proc = hierarchy.at(depth);
        switch (kind) {
        case Kind::Path:
            return proc.exePath == value;
        case Kind::Name:
            return !proc.exePath.isEmpty() && QFileInfo(proc.exePath).fileName() == value;
        case Kind::Hash: {
            // an unsupported algorithm yields an empty digest (and a log entry),
            // failing the condition
            const auto hash = client.exeHash(depth, algo);
            return !hash.isEmpty() && hash == value;
        }
        }
        return false;
    }

    namespace
    {
        // Evaluates one rule, hash conditions last so hashing only happens once
        // everything cheap already matched. When @a mismatchedHashDepths is given,
        // all hash conditions are evaluated and the failing depths collected, so a
        // fingerprint change across several hierarchy levels is fully reported.
        // A rule without conditions never matches.
        bool ruleMatches(const MatchRule& rule, DBusClient& client, QSet<int>* mismatchedHashDepths = nullptr)
        {
            if (rule.conditions.isEmpty()) {
                return false;
            }
            QList<const RuleCondition*> hashConditions;
            for (const auto& cond : rule.conditions) {
                if (cond.kind == RuleCondition::Kind::Hash) {
                    hashConditions << &cond;
                    continue;
                }
                if (!cond.matches(client)) {
                    return false;
                }
            }
            bool matched = true;
            for (const auto* cond : hashConditions) {
                if (!cond->matches(client)) {
                    matched = false;
                    if (!mismatchedHashDepths) {
                        break;
                    }
                    mismatchedHashDepths->insert(cond->depth);
                }
            }
            return matched;
        }
    } // namespace

    bool ClientRecord::matches(DBusClient& client) const
    {
        for (const auto& rule : rules) {
            if (ruleMatches(rule, client)) {
                return true;
            }
        }
        return false;
    }

    bool ClientRecord::fingerprintChanged(DBusClient& client, QSet<int>* mismatchedDepths) const
    {
        bool changed = false;
        for (const auto& rule : rules) {
            QSet<int> mismatched;
            if (ruleMatches(rule, client, &mismatched)) {
                return false;
            }
            if (!mismatched.isEmpty()) {
                changed = true;
                if (mismatchedDepths) {
                    *mismatchedDepths += mismatched;
                }
            }
        }
        return changed;
    }

    bool ClientRecord::operator==(const ClientRecord& other) const
    {
        return id == other.id && name == other.name && created == other.created && rules == other.rules
               && allEntries == other.allEntries;
    }

    bool ClientRecord::operator!=(const ClientRecord& other) const
    {
        return !(*this == other);
    }

    QString ClientRecord::customDataKey() const
    {
        return CustomData::getKeyWithPrefix(CustomData::FdoSecretsClientPrefix, id.toString(QUuid::WithoutBraces));
    }

    QString ClientRecord::toJson() const
    {
        QJsonArray rulesArr;
        for (const auto& rule : rules) {
            QJsonArray condArr;
            for (const auto& cond : rule.conditions) {
                QJsonObject condObj{
                    {QStringLiteral("depth"), cond.depth},
                    {QStringLiteral("kind"), kindToString(cond.kind)},
                    {QStringLiteral("value"), cond.value},
                };
                if (cond.kind == RuleCondition::Kind::Hash) {
                    condObj.insert(QStringLiteral("algo"), cond.algo);
                }
                condArr.append(condObj);
            }
            rulesArr.append(QJsonObject{{QStringLiteral("conditions"), condArr}});
        }
        QJsonObject obj{
            {QStringLiteral("version"), RecordVersion},
            {QStringLiteral("name"), name},
            {QStringLiteral("created"), created.toUTC().toString(Qt::ISODate)},
            {QStringLiteral("rules"), rulesArr},
            {QStringLiteral("allEntries"), decisionToString(allEntries)},
        };
        return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }

    ClientRecord ClientRecord::fromJson(const DBusClientId& id, const QString& json)
    {
        if (id.isNull()) {
            return {};
        }
        const auto doc = QJsonDocument::fromJson(json.toUtf8());
        if (!doc.isObject()) {
            return {};
        }
        const auto obj = doc.object();
        if (obj.value(QStringLiteral("version")).toInt() != RecordVersion) {
            return {};
        }

        ClientRecord record;
        record.id = id;
        record.name = obj.value(QStringLiteral("name")).toString();
        record.created = QDateTime::fromString(obj.value(QStringLiteral("created")).toString(), Qt::ISODate);
        record.allEntries = decisionFromString(obj.value(QStringLiteral("allEntries")).toString());

        // reject the whole record on any malformed condition: silently dropping a
        // condition would broaden what the record matches
        for (const auto& ruleVal : obj.value(QStringLiteral("rules")).toArray()) {
            MatchRule rule;
            for (const auto& condVal : ruleVal.toObject().value(QStringLiteral("conditions")).toArray()) {
                const auto condObj = condVal.toObject();
                RuleCondition cond;
                cond.depth = condObj.value(QStringLiteral("depth")).toInt(-1);
                cond.value = condObj.value(QStringLiteral("value")).toString();
                cond.algo = condObj.value(QStringLiteral("algo")).toString();
                if (cond.depth < 0 || cond.value.isEmpty()
                    || !kindFromString(condObj.value(QStringLiteral("kind")).toString(), cond.kind)) {
                    return {};
                }
                if ((cond.kind == RuleCondition::Kind::Hash) != !cond.algo.isEmpty()) {
                    return {};
                }
                rule.conditions.append(cond);
            }
            if (rule.conditions.isEmpty()) {
                return {};
            }
            record.rules.append(rule);
        }
        return record;
    }

    QList<ClientRecord> loadClientRecords(const Database* db)
    {
        QList<ClientRecord> records;
        const auto customData = db->metadata()->customData();
        for (const auto& key : customData->keys()) {
            if (!key.startsWith(CustomData::FdoSecretsClientPrefix)) {
                continue;
            }
            const auto id = QUuid::fromString(key.mid(CustomData::FdoSecretsClientPrefix.size()));
            auto record = ClientRecord::fromJson(id, customData->value(key));
            if (record.isValid()) {
                records.append(record);
            }
        }
        return records;
    }

    ClientRecord loadClientRecord(const Database* db, const DBusClientId& id)
    {
        if (id.isNull()) {
            return {};
        }
        const auto key =
            CustomData::getKeyWithPrefix(CustomData::FdoSecretsClientPrefix, id.toString(QUuid::WithoutBraces));
        return ClientRecord::fromJson(id, db->metadata()->customData()->value(key));
    }

    void saveClientRecord(Database* db, const ClientRecord& record)
    {
        Q_ASSERT(record.isValid());
        db->metadata()->customData()->set(record.customDataKey(), record.toJson());
    }

    void removeClientRecord(Database* db, const DBusClientId& id)
    {
        if (id.isNull()) {
            return;
        }
        const auto key =
            CustomData::getKeyWithPrefix(CustomData::FdoSecretsClientPrefix, id.toString(QUuid::WithoutBraces));
        db->metadata()->customData()->remove(key);
        for (auto entry : db->rootGroup()->entriesRecursive()) {
            setEntryClientDecision(entry, id, AuthDecision::Undecided);
        }
    }

    QHash<DBusClientId, AuthDecision> entryClientDecisions(const Entry* entry)
    {
        return entryClientDecisions(entry->customData());
    }

    void setEntryClientDecision(Entry* entry, const DBusClientId& id, AuthDecision decision)
    {
        setEntryClientDecision(entry->customData(), id, decision);
    }

    QHash<DBusClientId, AuthDecision> entryClientDecisions(const CustomData* customData)
    {
        QHash<DBusClientId, AuthDecision> decisions;
        const auto doc = QJsonDocument::fromJson(customData->value(EntryAuthKey).toUtf8());
        if (!doc.isObject() || doc.object().value(QStringLiteral("version")).toInt() != EntryAuthVersion) {
            return decisions;
        }
        const auto clients = doc.object().value(QStringLiteral("clients")).toObject();
        for (auto it = clients.begin(); it != clients.end(); ++it) {
            const auto id = QUuid::fromString(it.key());
            const auto decision = decisionFromString(it.value().toString());
            if (!id.isNull() && decision != AuthDecision::Undecided) {
                decisions.insert(id, decision);
            }
        }
        return decisions;
    }

    void setEntryClientDecision(CustomData* customData, const DBusClientId& id, AuthDecision decision)
    {
        if (id.isNull()) {
            return;
        }
        auto decisions = entryClientDecisions(customData);
        const auto persisted = decisionToString(decision);
        if (persisted.isEmpty()) {
            if (!decisions.remove(id)) {
                return;
            }
        } else {
            decisions.insert(id, decision);
        }

        if (decisions.isEmpty()) {
            customData->remove(EntryAuthKey);
            return;
        }
        QJsonObject clients;
        for (auto it = decisions.constBegin(); it != decisions.constEnd(); ++it) {
            clients.insert(it.key().toString(QUuid::WithoutBraces), decisionToString(it.value()));
        }
        const QJsonObject obj{
            {QStringLiteral("version"), EntryAuthVersion},
            {QStringLiteral("clients"), clients},
        };
        customData->set(EntryAuthKey, QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    }

    namespace
    {
        // deterministic pick among overlapping records: a denying catch-all wins,
        // then the earliest created record
        bool preferredOver(const ClientRecord& record, const ClientRecord& other)
        {
            const auto recordDenies = record.allEntries == AuthDecision::Denied;
            const auto otherDenies = other.allEntries == AuthDecision::Denied;
            if (recordDenies != otherDenies) {
                return recordDenies;
            }
            if (record.created != other.created) {
                return record.created < other.created;
            }
            return record.id < other.id;
        }
    } // namespace

    namespace
    {
        bool conditionsContradict(const RuleCondition& x, const RuleCondition& y)
        {
            if (x.depth != y.depth) {
                return false;
            }
            if (x.kind == y.kind) {
                // digests of different algorithms are unrelatable, anything else
                // of the same kind must agree on the value
                if (x.kind == RuleCondition::Kind::Hash && x.algo != y.algo) {
                    return false;
                }
                return x.value != y.value;
            }
            if (x.kind == RuleCondition::Kind::Path && y.kind == RuleCondition::Kind::Name) {
                return QFileInfo(x.value).fileName() != y.value;
            }
            if (x.kind == RuleCondition::Kind::Name && y.kind == RuleCondition::Kind::Path) {
                return QFileInfo(y.value).fileName() != x.value;
            }
            return false;
        }

        bool rulesMayOverlap(const MatchRule& a, const MatchRule& b)
        {
            // an empty rule never matches anything
            if (a.conditions.isEmpty() || b.conditions.isEmpty()) {
                return false;
            }
            // pool both rules' conditions: a contradiction anywhere (including
            // within a single self-contradictory rule) means no client can
            // satisfy both rules at once
            QList<const RuleCondition*> all;
            for (const auto& cond : a.conditions) {
                all << &cond;
            }
            for (const auto& cond : b.conditions) {
                all << &cond;
            }
            for (int i = 0; i < all.size(); ++i) {
                for (int j = i + 1; j < all.size(); ++j) {
                    if (conditionsContradict(*all.at(i), *all.at(j))) {
                        return false;
                    }
                }
            }
            return true;
        }
    } // namespace

    bool recordsOverlap(const ClientRecord& a, const ClientRecord& b)
    {
        for (const auto& ruleA : a.rules) {
            for (const auto& ruleB : b.rules) {
                if (rulesMayOverlap(ruleA, ruleB)) {
                    return true;
                }
            }
        }
        return false;
    }

    ClientResolution resolveClient(const Database* db, DBusClient& client)
    {
        ClientResolution res;
        const ClientRecord* best = nullptr;
        const ClientRecord* bestChanged = nullptr;
        QSet<int> bestMismatched;
        const auto records = loadClientRecords(db);
        for (const auto& record : records) {
            if (record.matches(client)) {
                if (!best || preferredOver(record, *best)) {
                    best = &record;
                }
                continue;
            }
            QSet<int> mismatched;
            if (record.fingerprintChanged(client, &mismatched)) {
                if (!bestChanged || preferredOver(record, *bestChanged)) {
                    bestChanged = &record;
                    bestMismatched = mismatched;
                }
            }
        }
        if (best) {
            res.record = *best;
        } else if (bestChanged) {
            res.fingerprintChanged = true;
            res.changed = *bestChanged;
            res.mismatchedDepths = bestMismatched;
        }
        return res;
    }

    AuthDecision persistedDecision(const Entry* entry, DBusClient& client)
    {
        const auto db = entry->database();
        if (!db) {
            return AuthDecision::Undecided;
        }
        const auto res = resolveClient(db, client);
        if (!res.record.isValid()) {
            return AuthDecision::Undecided;
        }
        const auto perEntry = entryClientDecisions(entry);
        const auto it = perEntry.constFind(res.record.id);
        if (it != perEntry.constEnd()) {
            return *it;
        }
        return res.record.allEntries;
    }

    bool isGenericClient(const QString& exePath)
    {
        // interpreters and shells run whatever they are handed, and command
        // line clients ask on behalf of whoever called them. The list is not
        // exhaustive and cannot be: it catches the common cases so the warning
        // appears where it is most needed.
        static const QSet<QString> generic{
            QStringLiteral("sh"),        QStringLiteral("bash"),   QStringLiteral("dash"),
            QStringLiteral("zsh"),       QStringLiteral("fish"),   QStringLiteral("ksh"),
            QStringLiteral("csh"),       QStringLiteral("tcsh"),   QStringLiteral("python"),
            QStringLiteral("perl"),      QStringLiteral("ruby"),   QStringLiteral("node"),
            QStringLiteral("deno"),      QStringLiteral("bun"),    QStringLiteral("lua"),
            QStringLiteral("luajit"),    QStringLiteral("php"),    QStringLiteral("tclsh"),
            QStringLiteral("expect"),    QStringLiteral("java"),   QStringLiteral("secret-tool"),
            QStringLiteral("dbus-send"), QStringLiteral("busctl"), QStringLiteral("gdbus"),
            QStringLiteral("qdbus"),     QStringLiteral("xargs"),  QStringLiteral("env"),
        };
        auto base = QFileInfo(exePath).fileName();
        // fold versioned names like python3.11, php8 or lua5.4 onto their stem
        static const QRegularExpression versionSuffix(QStringLiteral("-?[0-9][0-9.]*$"));
        base.remove(versionSuffix);
        return generic.contains(base);
    }

    MatchRule buildMatchRule(DBusClient& client, QSet<int> depths)
    {
        const auto& hierarchy = client.processInfo().hierarchy;
        auto sorted = depths.values();
        std::sort(sorted.begin(), sorted.end());

        MatchRule rule;
        for (const auto depth : sorted) {
            if (depth < 0 || depth >= hierarchy.size()) {
                continue;
            }
            // the executable content is anchorable even when the path is not,
            // e.g. for a binary that was deleted while the process is running
            if (!hierarchy.at(depth).exePath.isEmpty()) {
                rule.conditions.append({depth, RuleCondition::Kind::Path, hierarchy.at(depth).exePath, {}});
            }
            const auto hash = client.exeHash(depth, DefaultExeHashAlgo);
            if (!hash.isEmpty()) {
                rule.conditions.append({depth, RuleCondition::Kind::Hash, hash, DefaultExeHashAlgo});
            }
        }
        return rule;
    }

    DBusClientId upsertClientRecord(Database* db, DBusClient& client, const QSet<int>& matchDepths)
    {
        const auto res = resolveClient(db, client);
        if (res.record.isValid()) {
            return res.record.id;
        }

        if (res.fingerprintChanged) {
            // the user re-decided after seeing the change warning: re-anchor the
            // stale hashes of every rule that still identifies the client
            auto record = res.changed;
            for (auto& rule : record.rules) {
                const auto identified =
                    std::all_of(rule.conditions.cbegin(), rule.conditions.cend(), [&client](const RuleCondition& cond) {
                        return cond.kind == RuleCondition::Kind::Hash || cond.matches(client);
                    });
                if (!identified) {
                    continue;
                }
                for (auto& cond : rule.conditions) {
                    if (cond.kind != RuleCondition::Kind::Hash) {
                        continue;
                    }
                    const auto hash = client.exeHash(cond.depth, cond.algo);
                    if (!hash.isEmpty()) {
                        cond.value = hash;
                    }
                }
            }
            saveClientRecord(db, record);
            return record.id;
        }

        auto rule = buildMatchRule(client, matchDepths);
        if (rule.conditions.isEmpty()) {
            return {};
        }
        ClientRecord record;
        record.id = QUuid::createUuid();
        record.name = client.name();
        record.created = Clock::currentDateTimeUtc();
        record.rules << rule;
        saveClientRecord(db, record);
        return record.id;
    }
} // namespace FdoSecrets
