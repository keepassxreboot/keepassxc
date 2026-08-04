# Persistent Windows Hello Unlock: Windows Testing

This proof of concept must be built with MSVC on a real Windows 11 system. The Windows Hello implementation is only
selected for MSVC builds and requires the Windows SDK `WindowsApp.lib`. A MinGW-only build does not exercise it.

## Prerequisites

* Windows 11 with Windows Hello PIN configured; fingerprint/face hardware is optional.
* Visual Studio 2022 17.x with **Desktop development with C++** and a current Windows 11 SDK.
* CMake 3.16 or newer.
* Git and vcpkg. KeePassXC's `INSTALL.md` requires vcpkg for MSVC builds.
* Qt and other dependencies resolvable through the selected vcpkg triplet.

Open **Developer PowerShell for VS 2022** in the repository root and set `VCPKG_ROOT` to the local vcpkg checkout.
The repository currently has no checked-in `CMakePresets.json`, so use an explicit build directory:

```powershell
$env:VCPKG_ROOT = 'C:\src\vcpkg'
cmake -S . -B build-windows-poc -G 'Visual Studio 17 2022' -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DWITH_TESTS=ON `
  -DWITH_GUI_TESTS=ON `
  -DKPXC_FEATURE_DOCS=OFF
cmake --build build-windows-poc --config Debug --target keepassxc testpersistentquickunlock
ctest --test-dir build-windows-poc -C Debug -R testpersistentquickunlock --output-on-failure
cmake --build build-windows-poc --config Debug --target format
```

Then run the broader test suite:

```powershell
ctest --test-dir build-windows-poc -C Debug -j $env:NUMBER_OF_PROCESSORS --output-on-failure
```

Confirm during CMake configuration that `WINSDK` resolves to `WindowsApp.lib`. The prototype is unavailable and its
checkbox remains hidden if the Windows Hello backend is not linked.

The executable is normally at `build-windows-poc\src\Debug\keepassxc.exe` for the multi-config Visual Studio
generator.

## Manual test cases

Use newly created throwaway KDBX4 databases and synthetic credentials. Do not use a real password database.

1. **Default unchanged**: open a database without selecting the new checkbox, close KeePassXC fully,
   relaunch, and confirm the master password is required. Locking without exiting must retain ordinary session Quick
   Unlock behavior.
2. **Enroll and restart**: open the database with its password, select **Allow Windows Hello unlock after restarting
   KeePassXC on this device**, approve Windows Hello, exit all KeePassXC processes, relaunch, and unlock from the Quick
   Unlock page. Confirm a Windows Hello prompt appears before the database opens.
3. **PIN fallback**: repeat case 2 using Windows Hello PIN rather than biometrics. Confirm it succeeds.
4. **Cancel/fallback**: cancel the Windows Hello prompt. Confirm KeePassXC switches to the normal credentials page,
   does not attempt to open the database, and accepts the master password. Lock again and confirm the persistent
   entry was not silently deleted.
5. **Forget**: select **Forget Windows Hello unlock on this device**, exit and relaunch. Confirm the master password
   is required and the `.pqu` record has been removed.
6. **Credential change**: enroll, then change the database master password in Database Settings. Save, exit, and
   relaunch. Confirm the old persistent entry is gone and the new master password works.
7. **Wrong database**: enroll two databases. Close the application and swap the two `.pqu` filenames
   below `%LOCALAPPDATA%\KeePassXC\persistent-quick-unlock` while KeePassXC is not running. Confirm neither database
   receives an unlock key and both fall back to the password page.
8. **Tamper/corruption**: flip a byte in a `.pqu` file. Confirm Windows Hello approval is followed by a closed failure
   and password fallback, without a database-open attempt or secret-bearing log output.
9. **Key-file credentials**: enroll password-plus-key-file and key-file-only databases. After enrollment, move the
   key file aside and confirm Windows Hello can restore the persisted effective key. Forgetting the record must make
   the original key file necessary again.
10. **Challenge-response credentials**: enroll a database using a hardware challenge-response key. Confirm Windows
    Hello restores the slot metadata but the physical device is still required and challenged during database open.
11. **Empty credentials**: enroll a throwaway database with an empty password or empty composite key. Confirm Windows
    Hello gates this KeePassXC unlock path, then confirm and document that the KDBX file itself remains unprotected
    when opened by software that does not implement this feature.
12. **KDBX3 boundary**: try enrollment on a KDBX3 database. Confirm KeePassXC reports that KDBX4 is required and does
    not create a record.
13. **Hello reset/loss**: enroll, reset the Windows Hello PIN/key material according to Windows test policy, and try
    again. Confirm clean password fallback and recovery with the master password.
14. **Process/memory lifecycle**: use Process Explorer or an equivalent debugger to verify the protected record
    remains after normal exit while no KeePassXC process remains. Review logs and `%TEMP%` for absence of password or
    serialized-key material.

## Windows-specific checks still required

The following cannot be established by the platform-independent fake backend and require the real system above:

* C++/WinRT header and `WindowsApp.lib` compilation with the repository's supported Windows SDK versions.
* Windows Hello key creation, reopen, signing consistency across a full process restart, PIN reset, and OS restart.
* One prompt per persistent protection/unprotection operation and correct foreground/focus behavior.
* Correct handling of every `KeyCredentialStatus`, especially cancel, not-found, disabled, and credential-reset cases.
* Confirmation that the Windows-managed private key is device/user bound and non-exportable on the tested hardware;
  record whether TPM-backed or software-backed protection is used.
* `%LOCALAPPDATA%` path resolution, inherited NTFS ACLs, atomic replacement, deletion, and non-roaming behavior in
  local, Microsoft-account, domain, and roaming-profile test environments.
* Real AES-256-GCM tamper detection and memory inspection after success, cancellation, and failure.
* Interaction with existing session Quick Unlock, screen lock, database lock/close, multi-database tabs, key files,
  YubiKey/OnlyKey, browser-triggered unlock, and application shutdown.
* UI layout, accessibility names, translations, and behavior at common DPI/scaling settings.

Passing the fake-backend tests alone must not be reported as successful Windows Hello testing.
