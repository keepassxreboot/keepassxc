# Persistent Windows Hello Unlock Prototype

Status: experimental proof of concept, Windows/MSVC only.

## Context

KeePassXC Quick Unlock currently serializes the active `CompositeKey`, protects it with the platform Quick Unlock
provider, and retains the protected value only in process memory on Windows. The value is removed when KeePassXC
exits. A normal password unlock remains the only way to open the database after a process restart.

The requested prototype must preserve that existing session behavior while allowing a KDBX4 database to
opt in to a Windows Hello protected unlock across restarts. It must not alter KDBX serialization.

## Decision

The prototype adds a small platform-independent persistent-unlock coordinator, an injectable protection backend,
and a Windows Hello backend. The unlock dialog exposes a Windows-only, unchecked opt-in on the normal credential
page. Existing Quick Unlock remains enabled and session-scoped by default.

### Stored payload

The plaintext payload is the existing serialized `CompositeKey`. It does **not** contain the master-password text.
Password and key-file components contain their normalized 32-byte effective keys. Challenge-response components
contain only their device-slot metadata; the physical device must still answer the database-specific challenge.

Before protection the coordinator wraps the serialized key in a versioned envelope containing:

* a fixed format marker and version;
* the KDBX4 public database UUID;
* a payload-type marker; and
* the serialized `CompositeKey`.

The coordinator rejects malformed envelopes, UUID mismatches, unsupported versions or payload types, trailing data,
oversized values, unknown key component types, and non-canonical serialized keys. Version 2 accepts empty keys,
passwords, key files, challenge-response slots, and their supported combinations; version 1 password-only records
remain readable.

### Supported database-key compositions

The persistence layer deliberately operates on KeePassXC's canonical `CompositeKey` serialization instead of on a
password-specific representation. It currently accepts:

* an empty composite key;
* a password component, including the effective key for an empty password;
* a static key-file component;
* a challenge-response device slot; and
* supported combinations of those components in their original order.

Unknown component UUIDs, malformed static keys, and data that does not round-trip to the same canonical
serialization are rejected before storage or use. An empty composite key can be gated by Windows Hello inside this
KeePassXC build, but the underlying KDBX file remains unprotected and can still be opened without Windows Hello by
other software.

Persisting a key-file component stores its normalized effective key, not the original file contents. Consequently,
the original key file is no longer required after Windows Hello approval. This changes the key file from an
independently required factor into material recoverable through Windows Hello. Challenge-response devices behave
differently: only their slot metadata is serialized, so the physical device must still answer the live,
database-specific challenge on every unlock.

### Windows protection and persistence

The Windows backend reuses the existing Windows Hello `KeyCredentialManager` / `KeyCredential.RequestSignAsync`
flow and KeePassXC cryptography. A fresh random challenge is signed after Windows Hello user verification. The
SHA-256 hash of that response is used with KeePassXC AES-256-GCM to protect the envelope. Windows Hello permits the
OS-configured PIN as well as biometrics; the prototype does not try to impose a biometric-only policy.

Only `challenge || authenticated-ciphertext` is written, atomically, beneath
`QStandardPaths::AppLocalDataLocation/persistent-quick-unlock`. On Windows this resolves below the current user's
local application-data directory rather than roaming application data. The Windows Hello private key is managed by
Windows, is unavailable to KeePassXC, and is unique to the current user/device credential. No payload is placed in
KeePassXC configuration, command-line arguments, logs, temporary files, the KDBX file, or cloud storage by this
feature.

Every unprotect operation calls Windows Hello. Cancellation or failure returns to the normal credential page and
does not attempt database opening with partial data.

### Database binding

KDBX4 `KPXC_PUBLIC_UUID` is used instead of the editable file path. KeePassXC already places this UUID in the public
KDBX4 header; a successful database open authenticates that header with the KDBX header HMAC. The UUID is used both
as the local record name and inside the encrypted envelope. Copying or renaming a protected record to another UUID
therefore fails before a database key is returned. Supplying a protected key to another database also fails at the
normal KDBX credential/HMAC check.

KDBX3 uses a path-derived UUID and is intentionally excluded from this prototype. A byte-for-byte database copy
retains the same UUID and is considered the same database identity.

### Deletion and invalidation

* The opt-in is inactive until a regular unlock succeeds and the user checks the persistent option.
* Choosing the existing reset/forget action removes both the session entry and the persistent record.
* Changing database credentials removes both entries before the new credentials are used.
* Unknown or malformed key components are never persisted.
* Corrupt, unauthentic, wrongly bound, or undecryptable records fail closed and are never passed to `Database::open`.
* Normal Windows Hello cancellation leaves the record in place so the master password remains a fallback.

### Threats and limitations

This convenience feature deliberately reduces protection from "knowledge of the master password" to control of the
Windows account/device plus successful Windows Hello verification. Windows Hello may use PIN fallback and may use a
software-protected key if no suitable TPM is available. Malware running as the user can copy, delete, replay, or
replace ciphertext, invoke the Windows Hello prompt, or read the decrypted key from KeePassXC memory after approval.
The encrypted envelope detects replacement under a different UUID and GCM detects modification, but the prototype
does not maintain an anti-rollback counter. Full-disk encryption remains important. Loss or reset of the Windows
Hello credential makes the record unusable; the master password is the recovery mechanism.

The design has not received a security audit and is not suitable for production without review of the Windows Hello
key lifecycle, desktop-app isolation boundary, local file ACLs, memory clearing, and deterministic behavior across
Windows updates.
