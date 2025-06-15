This is a C++ based repository that uses Qt5 as a primary support and GUI library. This repository is for a password manager application that stores passwords 
and other highly sensitive information. The data format that passwords are stored is called KDBX which is a mixed binary and XML format that is fully encrypted 
at rest. This format is unpacked into a series of data structures: Database, Groups, and Entries. Please follow these guidelines when contributing:

## Code Standards

### Required Before Each Commit
- Run `cmake --build . --target format` before committing any changes to ensure proper code formatting
- This will run clang-format to ensure all code conforms to the style guide
- From the checkout directory, also run `./release-tool i18n lupdate` to update translation files

### Development Flow
- Setup Build Folder: `mkdir build; cd build`
- Configure: `cmake -G Ninja -DWITH_XC_ALL=ON -DWITH_GUI_TESTS=ON ..`
- Build: `cmake --build . -- -j $(nproc)`
- Test: `ctest`

## Repository Structure
- `docs/topics`: Documentation written in asciidoctor syntax
- `src/`: Main source code files are under this subdirectory
- `src/autotype`: Code that emulates a virtual keyboard to type into interfaces
- `src/browser`: Interface with the KeePassXC Browser Extension using a JSON-based protocol
- `src/cli`: Command Line Interface code
- `src/core`: Contains files that define the data model and other shared code structures
- `src/format`: Code for import/export and reading/writing of KDBX databases
- `src/fdosecrets`: freedesktop.org Secret Service interface code
- `src/quickunlock`: Quick unlock interfaces for various platforms
- `src/sshagent`: SSH Agent interface code to load private keys from the database into ssh-agent
- `tests/`: Test source code files
- `tests/gui`: GUI test source code files

## Key Guidelines
1. Follow C++20 and Qt5 best practices and idiomatic patterns
2. Maintain existing code structure and organization
3. Prefer not to edit cryptographic handling code or other sensitive parts of the code base
4. Write unit tests for new functionality using QTest scaffolding
5. Suggest changes to the `docs/topics` folder when appropriate
6. Unless the change is simple, don't actually make edits to .ui files, just suggest the changes needed
