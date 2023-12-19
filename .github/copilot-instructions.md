This repository is a C++ (C++20) Qt-based password manager. The important domain concepts are
Database, Group, and Entry (KDBX format). Key areas to know before making changes are below.

Quick reference (common commands)
- Configure + build (preferred: CMake presets)
	- Windows (PowerShell): `cmake --preset x64-debug`
	- Build: `cmake --build --preset x64-debug` or `cmake --build . -j <n>` from the build dir
- Formatting (required before commits):
	- `cmake --build . --target format` (runs clang-format)
- Tests:
	- Run all tests: `ctest -j <n>` from build dir
	- Run single test (verbose): `ctest -R <Test Name> -V`
- Translations & i18n (release tooling):
	- Update translation sources: `python ./release-tool.py i18n lupdate`

Big-picture architecture (where to look)
- src/core: core data model (Database, Groups, Entries). Example: `src/core/Database.h`
- src/format: KDBX readers/writers and import/export logic.  (sensitive - avoid casual edits)
- src/crypto: cryptographic primitives and key derivation. (sensitive - avoid casual edits)
- src/gui: Qt UI layers, widgets, main window and app lifecycle (entry: `src/main.cpp`, `src/gui/MainWindow.cpp`)
- src/sshagent, src/browser, src/fdosecrets, src/quickunlock: integration adapters for external systems
- tests/ and tests/gui/: QTest-based unit and GUI tests (follow existing test patterns)

Project-specific conventions & patterns
- Language/features: C++20, heavy use of Qt signal/slot idioms and QObject-derived classes.
- Build: use provided CMake commands to configure and build the project successfully.
- Formatting: a CMake target (`format`) runs clang-format — run it before committing.
- Translations: translation files are generated/updated via the release tool — run it before committing.
- UI files: .ui changes are non-trivial; prefer proposing .ui edits rather than committing wholesale .ui changes unless very simple.
- Sensitive areas: `src/crypto` and `src/format` contain security-sensitive logic — avoid refactors that change algorithms without expert review.

Concrete examples (where to copy patterns)
- Signal connections: see `src/keeshare/ShareObserver.cpp` (connect to Database signals like `groupAdded` / `modified`).
- Opening/locking DBs: `src/gui/DatabaseTabWidget.*` and `src/gui/DatabaseWidget.*` show typical lifecycle and `emitActiveDatabaseChanged()`.
- Format/validation: use `src/format/KdbxReader.cpp` and `Kdbx4Reader.cpp` for error handling patterns when reading DBs.

Rules for automated agents
- Do not change cryptographic or serialization logic unless the change is narrowly scoped and you run tests.
- When adding features, create relevant unit tests within existing files in `tests/`.
- Always run code formatting, translation update, and tests before submitting commits.
- All tests related to your change must pass before committing.
- Reference real files in PR descriptions (e.g., "changed src/core/Database.h and tests/TestDatabase.cpp").

If anything above is unclear or you want more detail about a specific area (build matrix, CI, or release-tool commands), tell me which part and I will expand.
