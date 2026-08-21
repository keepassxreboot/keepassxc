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
#include <QSet>
#include <QString>
#include <QUuid>

#include "core/Global.h"

class CustomData;
class Database;
class Entry;

namespace FdoSecrets
{
    class DBusClient;
    struct PeerInfo;

    inline bool decisionAllows(AuthDecision decision)
    {
        return decision == AuthDecision::Allowed || decision == AuthDecision::AllowedOnce;
    }

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

        /**
         * Whether the process at depth in the client's hierarchy satisfies this
         * condition. Anything unavailable or not understood (depth beyond the
         * hierarchy, unreadable executable, unknown hash algorithm) fails the
         * condition rather than skipping it.
         */
        bool matches(DBusClient& client) const;

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

        /**
         * Whether any rule fully matches the client. Hash conditions are evaluated
         * last so the (possibly expensive) hashing only happens when everything
         * else already matched.
         */
        bool matches(DBusClient& client) const;

        /**
         * Whether some rule identifies the client by all its non-hash conditions
         * while at least one of its hash conditions fails: the previously
         * authorized executable content has changed and the user must re-decide.
         * The failing depths of all such rules are added to @a mismatchedDepths.
         */
        bool fingerprintChanged(DBusClient& client, QSet<int>* mismatchedDepths = nullptr) const;

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

    /**
     * The same on a bare CustomData, for the entry editor, which stages its
     * edits on a copy of the entry's customData and writes it back on save.
     */
    QHash<DBusClientId, AuthDecision> entryClientDecisions(const CustomData* customData);
    void setEntryClientDecision(CustomData* customData, const DBusClientId& id, AuthDecision decision);

    struct ClientResolution
    {
        /// The single record the client resolved to; invalid when none matches.
        ClientRecord record;
        /// No record matches, but at least one identifies the client with a
        /// changed executable hash. Set alongside changed/mismatchedDepths.
        bool fingerprintChanged = false;
        /// The changed record (overlap resolved like regular matches).
        ClientRecord changed;
        QSet<int> mismatchedDepths;
    };

    /**
     * Best-effort static check whether two records can match the same client:
     * true unless some pair of their rules is contradiction-free. Conditions
     * contradict when they require different values of the same kind (or
     * different digests of the same algorithm) at the same depth, or a name
     * that is not the basename of a required path. Conditions at unshared
     * depths never contradict, so a record anchored only on an ancestor
     * overlaps everything below it. Used by the settings editor to flag
     * overlapping configurations; actual resolution stays deterministic via
     * resolveClient().
     */
    bool recordsOverlap(const ClientRecord& a, const ClientRecord& b);

    /**
     * Resolve @a client to at most one identity record persisted in @a db.
     * Overlapping records (a configuration smell, flagged in the editor) are
     * resolved deterministically: a denying catch-all wins, then the earliest
     * created record.
     */
    ClientResolution resolveClient(const Database* db, DBusClient& client);

    /**
     * The persisted decision for the <entry, client> pair: the entry's own
     * decision referencing the resolved record, falling back to the record's
     * catch-all. Undecided when the client resolves to no record.
     */
    AuthDecision persistedDecision(const Entry* entry, DBusClient& client);

    /**
     * Whether the executable is a general purpose tool that acts for whoever
     * invokes it — a script interpreter, a shell, or a command line client such
     * as secret-tool. A stored decision for one covers every use of it by any
     * program, which the access dialog warns about.
     */
    bool isGenericClient(const QString& exePath);

    /**
     * Build the rule anchoring @a client at the given hierarchy depths: for each
     * depth its executable path and content hash, either of which may be
     * unobtainable alone (a deleted binary loses its path but /proc/PID/exe still
     * hashes its content). Depths with neither contribute nothing; the rule is
     * empty (and never matches) when no depth is usable.
     */
    MatchRule buildMatchRule(DBusClient& client, QSet<int> depths);

    /**
     * Ensure @a db has an identity record for @a client and return its id:
     * an already matching record is used as is; a record whose fingerprint
     * changed gets its hash conditions updated in place; otherwise a new record
     * with the default rule over @a matchDepths is created. Returns a null id
     * when no record can anchor the client (no usable executable path).
     */
    DBusClientId upsertClientRecord(Database* db, DBusClient& client, const QSet<int>& matchDepths);
} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_CLIENTAUTH_H
