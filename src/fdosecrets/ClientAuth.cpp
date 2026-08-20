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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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
        QHash<DBusClientId, AuthDecision> decisions;
        const auto doc = QJsonDocument::fromJson(entry->customData()->value(EntryAuthKey).toUtf8());
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

    void setEntryClientDecision(Entry* entry, const DBusClientId& id, AuthDecision decision)
    {
        if (id.isNull()) {
            return;
        }
        auto decisions = entryClientDecisions(entry);
        const auto persisted = decisionToString(decision);
        if (persisted.isEmpty()) {
            if (!decisions.remove(id)) {
                return;
            }
        } else {
            decisions.insert(id, decision);
        }

        if (decisions.isEmpty()) {
            entry->customData()->remove(EntryAuthKey);
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
        entry->customData()->set(EntryAuthKey, QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    }

} // namespace FdoSecrets
