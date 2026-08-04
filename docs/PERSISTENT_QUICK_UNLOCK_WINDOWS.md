# Remembered Quick Unlock on Windows

Status: experimental Qt 6 port for review and Windows 11 testing.

## Scope

This branch ports the Windows portion of KeePassXC's `feature/remember-quickunlock` work to the current Qt 6
`develop` branch. It deliberately excludes PIN Quick Unlock, the unfinished Polkit interface, and cross-platform
secret-storage changes.

The application-wide **Remember quick unlock after database is closed** setting is available when the active Quick
Unlock provider supports persistence. It is disabled by default. When it is disabled, Windows Hello protected
session keys remain in process memory exactly as before and are removed when the database is closed. They are never
written to persistent storage, including during the interval before a clean shutdown.

When the setting is enabled, the Windows provider stores `challenge || AES-256-GCM ciphertext` in the current user's
Windows Password Vault. The AES key is derived from a Windows Hello signature over the random challenge. Every
unprotect operation requests a new Windows Hello verification. The Windows Hello private key remains managed by
Windows and is not available to KeePassXC.

## Database keys

The existing database-open path serializes and restores KeePassXC's `CompositeKey`; the Windows provider does not
interpret or restrict its components. Passwords (including an empty password), key files, challenge-response slot
metadata, empty composite keys, and supported combinations therefore follow the same serialization path. For
challenge-response databases, the physical device is still required to answer the live database challenge.

Remembering a key-file component makes its normalized effective key recoverable after Windows Hello verification;
the original key file is no longer an independently required factor for that unlock path. An empty database key
remains empty in the KDBX file: Windows Hello gates only this KeePassXC convenience path and does not add KDBX
protection that other applications must honor.

## Invalidation

Disabling Quick Unlock or remembered Quick Unlock removes both session and Password Vault records. Closing a
database removes its record unless remembered Quick Unlock is enabled. The existing database-key-change path also
resets Quick Unlock for that database. Corrupt or truncated records fail before a key is returned; AES-GCM detects
ciphertext modification.

## Security boundary

This feature trades knowledge of all original database-key components for control of the Windows account/device and
successful Windows Hello verification. Windows Hello may permit a PIN fallback and protection may be TPM-backed or
software-backed depending on the system. Malware in the user session can invoke prompts, copy or delete vault data,
and read restored key material from KeePassXC memory after approval. The master password and original key material
remain the recovery mechanism if Windows Hello credentials are reset or lost.

The implementation has not received a security audit. Password Vault behavior, Windows Hello credential lifecycle,
deletion failures, Windows updates, account changes, and supported Windows SDK versions require maintainer review and
testing on real Windows 11 systems.

## Verification performed

The branch configures and builds `KeePassXC.exe` with Qt 6.11.1, MSVC 2022 19.44, the Windows 11 SDK, Ninja, and the
vcpkg `x64-windows` triplet. `testconfig` verifies that remembered Quick Unlock defaults to disabled. Manual testing
must use throwaway KDBX4 databases and synthetic password, key-file, challenge-response, and empty-key credentials.

On 2026-08-04, a manual Windows 11 smoke test using an isolated KeePassXC configuration and a throwaway KDBX4
database verified both primary lifecycle paths:

* after a normal password unlock, Windows Hello enrollment, and a complete process exit, the database unlocked with
  Windows Hello after KeePassXC was restarted, without requesting the database password; and
* after remembered Quick Unlock was disabled and KeePassXC exited, reopening the same database displayed the normal
  password page and did not offer the previously persisted Windows Hello unlock.

This smoke test does not cover key files, challenge-response devices, empty keys, credential reset, record corruption,
or different Windows account and hardware configurations.

Additional manual testing verified that canceling Windows Hello does not open the database, delete the persisted
record, or force a password retry. The Quick Unlock screen remains available and a second Windows Hello attempt can
unlock the database. This test exposed and fixed a transient `WindowActivate` race that previously interpreted a
temporarily unavailable vault record as missing and deleted it.

Creating and saving a new throwaway KDBX4 database now triggers Windows Hello enrollment after the first successful
save. After a complete process restart, the new database was available through Quick Unlock without an intervening
password unlock.
