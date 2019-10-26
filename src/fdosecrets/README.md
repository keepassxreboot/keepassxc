# Freedesktop.org Secret Storage Spec Server Side API

This plugin implements the [Secret Storage specification][secrets] version 0.2. While running KeePassXC, it acts as a
Secret Service server, registered on DBus, so clients like seahorse, python-secretstorage, or other implementations
can connect and access the exposed database in KeePassXC.

[secrets]: (https://www.freedesktop.org/wiki/Specifications/secret-storage-spec/)

## Configurable settings

* The user can specify if a database is exposed on DBus, and which group is exposed.
* Whether to show desktop notification is shown when an entry is retrived.
* Whether to skip confirmation for entries deleted from DBus

## Implemented Attributes on Item Object

The following attributes are exposed:

|Key|Value|
|:---:|:---:|
|Title|The entry title|
|UserName|The entry user name|
|URL|The entry URL|
|Notes|The entry notes|

In addition, all non-protected custom attributes are also exposed.

## Architecture

* `FdoSecrets::Service` is the top level DBus service
* There is one and only one `FdoSecrets::Collection` per opened database tab
* Each entry under the exposed database group has a corresponding `FdoSecrets::Item` DBus object.

### Signal connections

- Collections are created when a corresponding database tab opened
- If the database is locked, a collection is created
- When the database is unlocked, collection populates its children
- If the unlocked database's exposed group is none, collection deletes itself
- If the database's exposed group changes, collection repopulates
- If the database's exposed group changes to none, collection deletes itself
- If the database's exposed group changes from none, the service recreates a collection
