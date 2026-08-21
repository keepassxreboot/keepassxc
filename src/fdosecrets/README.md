# Freedesktop.org Secret Storage Spec Server Side API

This plugin implements the [Secret Storage specification][secrets] version 0.2. While running KeePassXC, it acts as a
Secret Service server, registered on DBus, so clients like seahorse, python-secretstorage, or other implementations
can connect and access the exposed database in KeePassXC.

[secrets]: (https://www.freedesktop.org/wiki/Specifications/secret-storage-spec/)

## Configurable settings

* The user can specify if a database is exposed on DBus, and which group is exposed.
* Whether to show desktop notification is shown when an entry's secret is retrieved.
* Whether to confirm for entries deleted from DBus
* Whether to confirm each entry's access

## Implemented Attributes on Item Object

The following attributes are exposed:

|Key|Value|
|:---:|:---:|
|Title|The entry title|
|UserName|The entry user name|
|URL|The entry URL|
|Notes|The entry notes|
|TOTP|The TOTP code if the entry has one|

In addition, all non-protected custom attributes are also exposed.

## Implementation

* `FdoSecrets::Service` is the top level DBus service
* There is one and only one `FdoSecrets::Collection` per opened database tab
* Each entry under the exposed database group has a corresponding `FdoSecrets::Item` DBus object.

### Signal connections

Collection here means the `Collection` object in code. Not the logical concept "collection"
that the user interacts with.

- Collections are created when a corresponding database tab opened
- If the database is locked, a collection is still created
- When the database is unlocked, collection populates its children
- If the unlocked database's exposed group is none, collection deletes itself
- If the database's exposed group changes, collection repopulates
- If the database's exposed group changes to none, collection deletes itself
- If the database's exposed group changes from none, the service recreates a collection

## Client authorization

Access decisions can outlive the DBus connection they were made on. They are
stored in the database the entry belongs to, in two layers.

### Identity records

One record per logical client, in `Database::metadata()->customData()` under
`FDO_SECRETS_CLIENT_<DBusClientId>`, holding a compact JSON document: a display
name, a creation time, the matching rules, and a catch-all decision for entries
without one of their own. A `DBusClientId` is a UUID minted when the record is
created; entry-side decisions reference the client by it.

One key per record rather than one document listing all of them: metadata
customData merges as a whole, deciding per key by `_LAST_MODIFIED`, so a single
document would lose updates whenever two machines both authorized something.
The key prefix is registered in `CustomData::isProtected()` so a merge cannot
delete records that only exist on one side.

A rule is a conjunction of conditions and a record matches if any of its rules
does. A condition constrains one process of the client's hierarchy
(`PeerInfo::hierarchy`, index 0 being the caller) by executable path, by the
file name of that path, or by a digest of the executable content. Anything that
cannot be evaluated — a depth beyond the hierarchy, an unreadable executable, a
digest algorithm this build does not know — fails the condition instead of
being skipped, so an unusable rule never widens access.

### Entry decisions

Each entry carries its own decisions in its customData under
`FDO_SECRETS_AUTH`: a JSON object mapping DBusClientId to `allow`/`deny`. They
live on the entry so they follow it through moves, deletion and merges instead
of leaving dangling uuids behind in the metadata, and so that a merge decides
them together with the rest of the entry. Ids of records that no longer exist
are ignored on read; removing a record sweeps them.

Neither layer is visible over DBus: `Item::attributes()` exposes entry
attributes, not customData.

## Matching

Answering "may this client read this entry" is two lookups, not a scan:

1. **Resolution** maps the client to at most one record, by evaluating the
   records of that database against the client's process hierarchy. Records
   that can match the same client are a configuration smell rather than an
   error, so resolution is deterministic: a record whose catch-all denies wins,
   otherwise the one created first. The settings page warns about the overlap.
2. **Decision** looks the resolved DBusClientId up in the entry's own
   decisions, falling back to the record's catch-all.

Resolution deliberately has no cache. Records are looked up by a customData key
whose `_LAST_MODIFIED` timestamp has second resolution, which is not enough to
notice that a record changed within the same second — and the failure mode of a
stale cache here is honoring an authorization the user just revoked.

A record whose non-digest conditions all hold while a digest condition fails
identifies the client with changed executable content. That is not a match: the
client is asked again, with the changed processes marked, and re-authorizing
updates the digests of the existing record instead of adding a second one.

### Hashing

Digests are taken from `/proc/<pid>/exe`, which is the original inode: content
is read even when the path has since been replaced or deleted. Reading fails
for a process that is gone, owned by another user, or has `PR_SET_DUMPABLE`
cleared, and a failure fails the condition.

Hashing is lazy — conditions that do not need it are evaluated first, so a
client only pays for it when everything cheaper about a record already matched
— and results are cached per connection, including failures: a live process
cannot change its `/proc/<pid>/exe`, and a reused pid must not be able to
produce a different digest than the process first seen at that depth.

## Runtime state versus stored decisions

`DBusClient` holds what the current connection decided, nothing more: the
decisions that apply to this request only, plus the ones just granted for
entries the client itself created. Stored decisions are never copied into it,
which is what keeps a catch-all from leaking from the database it was granted
in to another one.

`Service::authDecision()` is the only place the two are combined. Denials win
within a layer, and a decision limited to this request wins over a stored one,
so answering a prompt with Remember unchecked cannot be overruled by what the
database holds.
