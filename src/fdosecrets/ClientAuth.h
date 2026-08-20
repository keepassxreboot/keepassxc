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

#ifndef KEEPASSXC_FDOSECRETS_CLIENTAUTH_H
#define KEEPASSXC_FDOSECRETS_CLIENTAUTH_H

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>
#include <QUuid>

#include "core/Global.h"

class Database;
class Entry;

namespace FdoSecrets
{
    /// The hash algorithm newly created rules use. Stored rules carry their own
    /// algorithm name, so this can change without invalidating them.
    extern const QString DefaultExeHashAlgo;

    /**
     * Stable identifier of a logical client application, assigned when its identity
     * record is first persisted. Entry-side decisions reference records by this id.
     */
    using DBusClientId = QUuid;

    /**
     * A single requirement on one process in the calling client's process hierarchy.
     */
    struct RuleCondition
    {
        enum class Kind
        {
            Path, ///< exact match on the canonical executable path
            Name, ///< exact match on the basename of the executable path
            Hash, ///< match on the digest of the executable content read from /proc/PID/exe
        };

        /// Index into PeerInfo.hierarchy: 0 is the calling process itself, 1 its parent, ...
        int depth = 0;
        Kind kind = Kind::Path;
        /// Path/Name: the exact string to compare. Hash: lowercase hex digest.
        QString value;
        /// Hash only. Currently always "sha256".
        QString algo;

        bool operator==(const RuleCondition& other) const;
        bool operator!=(const RuleCondition& other) const;
    };

    /**
     * A conjunction of conditions: the rule matches iff every condition holds.
     */
    struct MatchRule
    {
        QList<RuleCondition> conditions;

        bool operator==(const MatchRule& other) const;
        bool operator!=(const MatchRule& other) const;
    };

    /**
     * Identity record of a logical client, persisted in the database's metadata
     * customData under one key per record. A live client resolves to at most one
     * record (any rule matching means the record matches); per-entry decisions are
     * stored on the entries themselves, keyed by the record's DBusClientId.
     */
    struct ClientRecord
    {
        DBusClientId id;
        /// Human readable display name, taken from the client name at creation.
        QString name;
        QDateTime created;
        /// Disjunction: the record matches if any rule matches.
        QList<MatchRule> rules;
        /// Catch-all decision for entries without a per-entry decision.
        /// Only Undecided/Allowed/Denied are meaningful here.
        AuthDecision allEntries = AuthDecision::Undecided;

        bool isValid() const
        {
            return !id.isNull();
        }

        /// The metadata customData key this record is stored under.
        QString customDataKey() const;

        QString toJson() const;
        static ClientRecord fromJson(const DBusClientId& id, const QString& json);

        bool operator==(const ClientRecord& other) const;
        bool operator!=(const ClientRecord& other) const;
    };

    /**
     * Storage of client identity records in Database::metadata()->customData().
     */
    QList<ClientRecord> loadClientRecords(const Database* db);
    ClientRecord loadClientRecord(const Database* db, const DBusClientId& id);
    void saveClientRecord(Database* db, const ClientRecord& record);
    /**
     * Remove the record and sweep all entries in @a db for decisions referencing it.
     */
    void removeClientRecord(Database* db, const DBusClientId& id);

    /**
     * Per-entry decisions, stored in the entry's own customData so they follow the
     * entry through moves, deletion and merges. Referenced records may no longer
     * exist; such stale ids are ignored on read.
     */
    QHash<DBusClientId, AuthDecision> entryClientDecisions(const Entry* entry);
    /**
     * Set or update one decision on @a entry. Only Allowed/Denied are persisted;
     * Undecided removes the decision (and the customData key when none are left).
     */
    void setEntryClientDecision(Entry* entry, const DBusClientId& id, AuthDecision decision);

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_CLIENTAUTH_H
