/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "TestCli.h"

#include "config-keepassx-tests.h"
#include "core/Bootstrap.h"
#include "core/Config.h"
#include "core/CustomData.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "keys/FileKey.h"
#include "keys/drivers/YubiKey.h"
#include "mock/MockClock.h"

#include "cli/Add.h"
#include "cli/AddGroup.h"
#include "cli/Analyze.h"
#include "cli/AttachmentExport.h"
#include "cli/AttachmentImport.h"
#include "cli/AttachmentRemove.h"
#include "cli/Clip.h"
#include "cli/DatabaseCreate.h"
#include "cli/DatabaseEdit.h"
#include "cli/DatabaseInfo.h"
#include "cli/Diceware.h"
#include "cli/Edit.h"
#include "cli/Estimate.h"
#include "cli/Export.h"
#include "cli/Generate.h"
#include "cli/Help.h"
#include "cli/Import.h"
#include "cli/List.h"
#include "cli/Merge.h"
#include "cli/Move.h"
#include "cli/MoveDir.h"
#include "cli/Open.h"
#include "cli/Remove.h"
#include "cli/RemoveGroup.h"
#include "cli/Search.h"
#include "cli/Show.h"
#include "cli/TextStream.h"
#include "cli/Utils.h"

#include <QClipboard>
#include <QFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTest>
#include <QtConcurrent>
#include <qglobal.h>
#include <zxcvbn.h>

QTEST_MAIN(TestCli)

void TestCli::initTestCase()
{
    QVERIFY(Crypto::init());

    // Create temporary config file
    Config::createConfigFromFile(TemporaryFile::createTempConfigFile(), {});

    QLocale::setDefault(QLocale::c());
    Bootstrap::bootstrap();

    m_devNull.reset(new QFile());
#ifdef Q_OS_WIN
    m_devNull->open(fopen("nul", "w"), QIODevice::WriteOnly);
#else
    m_devNull->open(fopen("/dev/null", "w"), QIODevice::WriteOnly);
#endif
    Utils::DEVNULL.setDevice(m_devNull.data());
}

void TestCli::init()
{
    const auto file = QString(KEEPASSX_TEST_DATA_DIR).append("/%1");

    m_dbFile.reset(new TemporaryFile());
    m_dbFile->copyFromFile(file.arg("NewDatabase.kdbx"));

    m_dbFile2.reset(new TemporaryFile());
    m_dbFile2->copyFromFile(file.arg("NewDatabase2.kdbx"));

    m_dbFileMulti.reset(new TemporaryFile());
    m_dbFileMulti->copyFromFile(file.arg("NewDatabaseMulti.kdbx"));

    m_xmlFile.reset(new TemporaryFile());
    m_xmlFile->copyFromFile(file.arg("NewDatabase.xml"));

    m_keyFileProtectedDbFile.reset(new TemporaryFile());
    m_keyFileProtectedDbFile->copyFromFile(file.arg("KeyFileProtected.kdbx"));

    m_keyFileProtectedNoPasswordDbFile.reset(new TemporaryFile());
    m_keyFileProtectedNoPasswordDbFile->copyFromFile(file.arg("KeyFileProtectedNoPassword.kdbx"));

    m_yubiKeyProtectedDbFile.reset(new TemporaryFile());
    m_yubiKeyProtectedDbFile->copyFromFile(file.arg("YubiKeyProtectedPasswords.kdbx"));

    m_nonAsciiDbFile.reset(new TemporaryFile());
    m_nonAsciiDbFile->copyFromFile(file.arg("NonAscii.kdbx"));

    m_stdout.reset(new QBuffer());
    m_stdout->open(QIODevice::ReadWrite);
    Utils::STDOUT.setDevice(m_stdout.data());

    m_stderr.reset(new QBuffer());
    m_stderr->open(QIODevice::ReadWrite);
    Utils::STDERR.setDevice(m_stderr.data());

    m_stdin.reset(new QBuffer());
    m_stdin->open(QIODevice::ReadWrite);
    Utils::STDIN.setDevice(m_stdin.data());
}

void TestCli::cleanup()
{
    m_dbFile.reset();
    m_dbFile2.reset();
    m_dbFileMulti.reset();
    m_keyFileProtectedDbFile.reset();
    m_keyFileProtectedNoPasswordDbFile.reset();
    m_yubiKeyProtectedDbFile.reset();
}

void TestCli::cleanupTestCase()
{
    m_devNull.reset();
}

QSharedPointer<Database> TestCli::readDatabase(const QString& filename, const QString& pw, const QString& keyfile)
{
    auto db = QSharedPointer<Database>::create();
    auto key = QSharedPointer<CompositeKey>::create();

    if (filename.isEmpty()) {
        // Open the default test database
        key->addKey(QSharedPointer<PasswordKey>::create("a"));
        if (!db->open(m_dbFile->fileName(), key)) {
            return {};
        }
    } else {
        // Open the specified database file using supplied credentials
        key->addKey(QSharedPointer<PasswordKey>::create(pw));
        if (!keyfile.isEmpty()) {
            auto filekey = QSharedPointer<FileKey>::create();
            filekey->load(keyfile);
            key->addKey(filekey);
        }

        if (!db->open(filename, key)) {
            return {};
        }
    }

    return db;
}

int TestCli::execCmd(Command& cmd, const QStringList& args) const
{
    // Move to end of stream
    m_stdout->readAll();
    m_stderr->readAll();

    // Record stream position
    auto outPos = m_stdout->pos();
    auto errPos = m_stderr->pos();

    // Execute command
    int ret = cmd.execute(args);

    // Move back to recorded position
    m_stdout->seek(outPos);
    m_stderr->seek(errPos);

    // Skip over blank lines
    QByteArray newline("\n");
    while (m_stdout->peek(1) == newline) {
        m_stdout->readLine();
    }
    while (m_stderr->peek(1) == newline) {
        m_stderr->readLine();
    }

    return ret;
}

bool TestCli::isTotp(const QString& value)
{
    static const QRegularExpression totp("^\\d{6}$");
    return totp.match(value.trimmed()).hasMatch();
}

void TestCli::setInput(const QString& input)
{
    setInput(QStringList(input));
}

void TestCli::setInput(const QStringList& input)
{
    auto ba = input.join("\n").toLatin1();
    // Always end in newline
    if (!ba.endsWith("\n")) {
        ba.append("\n");
    }
    auto pos = m_stdin->pos();
    m_stdin->write(ba);
    m_stdin->seek(pos);
}

void TestCli::testBatchCommands()
{
    Commands::setupCommands(false);
    QVERIFY(Commands::getCommand("add"));
    QVERIFY(Commands::getCommand("analyze"));
    QVERIFY(Commands::getCommand("attachment-export"));
    QVERIFY(Commands::getCommand("attachment-import"));
    QVERIFY(Commands::getCommand("attachment-rm"));
    QVERIFY(Commands::getCommand("clip"));
    QVERIFY(Commands::getCommand("close"));
    QVERIFY(Commands::getCommand("db-create"));
    QVERIFY(Commands::getCommand("db-info"));
    QVERIFY(Commands::getCommand("diceware"));
    QVERIFY(Commands::getCommand("edit"));
    QVERIFY(Commands::getCommand("estimate"));
    QVERIFY(Commands::getCommand("export"));
    QVERIFY(Commands::getCommand("generate"));
    QVERIFY(Commands::getCommand("help"));
    QVERIFY(Commands::getCommand("import"));
    QVERIFY(Commands::getCommand("ls"));
    QVERIFY(Commands::getCommand("merge"));
    QVERIFY(Commands::getCommand("mkdir"));
    QVERIFY(Commands::getCommand("mv"));
    QVERIFY(Commands::getCommand("mvdir"));
    QVERIFY(Commands::getCommand("open"));
    QVERIFY(Commands::getCommand("rm"));
    QVERIFY(Commands::getCommand("rmdir"));
    QVERIFY(Commands::getCommand("show"));
    QVERIFY(Commands::getCommand("search"));
    QVERIFY(!Commands::getCommand("doesnotexist"));
    QCOMPARE(Commands::getCommands().size(), 27);
}

void TestCli::testInteractiveCommands()
{
    Commands::setupCommands(true);
    QVERIFY(Commands::getCommand("add"));
    QVERIFY(Commands::getCommand("analyze"));
    QVERIFY(Commands::getCommand("attachment-export"));
    QVERIFY(Commands::getCommand("attachment-import"));
    QVERIFY(Commands::getCommand("attachment-rm"));
    QVERIFY(Commands::getCommand("clip"));
    QVERIFY(Commands::getCommand("close"));
    QVERIFY(Commands::getCommand("db-create"));
    QVERIFY(Commands::getCommand("db-info"));
    QVERIFY(Commands::getCommand("diceware"));
    QVERIFY(Commands::getCommand("edit"));
    QVERIFY(Commands::getCommand("estimate"));
    QVERIFY(Commands::getCommand("exit"));
    QVERIFY(Commands::getCommand("generate"));
    QVERIFY(Commands::getCommand("help"));
    QVERIFY(Commands::getCommand("ls"));
    QVERIFY(Commands::getCommand("merge"));
    QVERIFY(Commands::getCommand("mkdir"));
    QVERIFY(Commands::getCommand("mv"));
    QVERIFY(Commands::getCommand("mvdir"));
    QVERIFY(Commands::getCommand("open"));
    QVERIFY(Commands::getCommand("quit"));
    QVERIFY(Commands::getCommand("rm"));
    QVERIFY(Commands::getCommand("rmdir"));
    QVERIFY(Commands::getCommand("show"));
    QVERIFY(Commands::getCommand("search"));
    QVERIFY(!Commands::getCommand("doesnotexist"));
    QCOMPARE(Commands::getCommands().size(), 27);
}

void TestCli::testAdd()
{
    Add addCmd;
    QVERIFY(!addCmd.name.isEmpty());
    QVERIFY(addCmd.getDescriptionLine().contains(addCmd.name));

    setInput("a");
    execCmd(addCmd,
            {"add",
             "-u",
             "newuser",
             "--url",
             "https://example.com/",
             "-g",
             "-L",
             "20",
             "--notes",
             "some notes",
             m_dbFile->fileName(),
             "/newuser-entry"});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QVERIFY(m_stdout->readAll().contains("Successfully added entry newuser-entry."));

    auto db = readDatabase();
    auto* entry = db->rootGroup()->findEntryByPath("/newuser-entry");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://example.com/"));
    QCOMPARE(entry->password().size(), 20);
    QCOMPARE(entry->notes(), QString("some notes"));

    // Quiet option
    setInput("a");
    execCmd(addCmd, {"add", "-q", "-u", "newuser", "-g", "-L", "20", m_dbFile->fileName(), "/newentry-quiet"});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray());
    db = readDatabase();
    entry = db->rootGroup()->findEntryByPath("/newentry-quiet");
    QVERIFY(entry);
    QCOMPARE(entry->password().size(), 20);

    setInput({"a", "newpassword"});
    execCmd(addCmd,
            {"add", "-u", "newuser2", "--url", "https://example.net/", "-p", m_dbFile->fileName(), "/newuser-entry2"});
    QVERIFY(m_stdout->readAll().contains("Successfully added entry newuser-entry2."));

    db = readDatabase();
    entry = db->rootGroup()->findEntryByPath("/newuser-entry2");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser2"));
    QCOMPARE(entry->url(), QString("https://example.net/"));
    QCOMPARE(entry->password(), QString("newpassword"));

    // Password generation options
    setInput("a");
    execCmd(addCmd, {"add", "-u", "newuser3", "-g", "-L", "34", m_dbFile->fileName(), "/newuser-entry3"});
    QVERIFY(m_stdout->readAll().contains("Successfully added entry newuser-entry3."));

    db = readDatabase();
    entry = db->rootGroup()->findEntryByPath("/newuser-entry3");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser3"));
    QCOMPARE(entry->password().size(), 34);
    QRegularExpression defaultPasswordClassesRegex("^[a-zA-Z0-9]+$");
    QVERIFY(defaultPasswordClassesRegex.match(entry->password()).hasMatch());

    setInput("a");
    execCmd(addCmd,
            {"add",
             "-u",
             "newuser4",
             "-g",
             "-L",
             "20",
             "--every-group",
             "-s",
             "-n",
             "-U",
             "-l",
             m_dbFile->fileName(),
             "/newuser-entry4"});
    QVERIFY(m_stdout->readAll().contains("Successfully added entry newuser-entry4."));

    db = readDatabase();
    entry = db->rootGroup()->findEntryByPath("/newuser-entry4");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser4"));
    QCOMPARE(entry->password().size(), 20);
    QVERIFY(!defaultPasswordClassesRegex.match(entry->password()).hasMatch());

    setInput("a");
    execCmd(addCmd, {"add", "-u", "newuser5", "--notes", "test\\nnew line", m_dbFile->fileName(), "/newuser-entry5"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray(""));
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully added entry newuser-entry5.\n"));

    db = readDatabase();
    entry = db->rootGroup()->findEntryByPath("/newuser-entry5");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser5"));
    QCOMPARE(entry->notes(), QString("test\nnew line"));
}

void TestCli::testAddGroup()
{
    AddGroup addGroupCmd;
    QVERIFY(!addGroupCmd.name.isEmpty());
    QVERIFY(addGroupCmd.getDescriptionLine().contains(addGroupCmd.name));

    setInput("a");
    execCmd(addGroupCmd, {"mkdir", m_dbFile->fileName(), "/new_group"});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully added group new_group.\n"));

    auto db = readDatabase();
    auto* group = db->rootGroup()->findGroupByPath("new_group");
    QVERIFY(group);
    QCOMPARE(group->name(), QString("new_group"));

    // Trying to add the same group should fail.
    setInput("a");
    execCmd(addGroupCmd, {"mkdir", m_dbFile->fileName(), "/new_group"});
    QVERIFY(m_stderr->readAll().contains("Group /new_group already exists!"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Should be able to add groups down the tree.
    setInput("a");
    execCmd(addGroupCmd, {"mkdir", m_dbFile->fileName(), "/new_group/newer_group"});
    QVERIFY(m_stdout->readAll().contains("Successfully added group newer_group."));

    db = readDatabase();
    group = db->rootGroup()->findGroupByPath("new_group/newer_group");
    QVERIFY(group);
    QCOMPARE(group->name(), QString("newer_group"));

    // Should fail if the path is invalid.
    setInput("a");
    execCmd(addGroupCmd, {"mkdir", m_dbFile->fileName(), "/invalid_group/newer_group"});
    QVERIFY(m_stderr->readAll().contains("Group /invalid_group not found."));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Should fail to add the root group.
    setInput("a");
    execCmd(addGroupCmd, {"mkdir", m_dbFile->fileName(), "/"});
    QVERIFY(m_stderr->readAll().contains("Group / already exists!"));
    QCOMPARE(m_stdout->readAll(), QByteArray());
}

void TestCli::testAnalyze()
{
    Analyze analyzeCmd;
    QVERIFY(!analyzeCmd.name.isEmpty());
    QVERIFY(analyzeCmd.getDescriptionLine().contains(analyzeCmd.name));

    const QString hibpPath = QString(KEEPASSX_TEST_DATA_DIR).append("/hibp.txt");

    setInput("a");
    execCmd(analyzeCmd, {"analyze", "--hibp", hibpPath, m_dbFile->fileName()});
    auto output = m_stdout->readAll();
    QVERIFY(output.contains("Sample Entry"));
    QVERIFY(output.contains("123"));
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
}

void TestCli::testAttachmentExport()
{
    AttachmentExport attachmentExportCmd;
    QVERIFY(!attachmentExportCmd.name.isEmpty());
    QVERIFY(attachmentExportCmd.getDescriptionLine().contains(attachmentExportCmd.name));

    TemporaryFile exportOutput;
    exportOutput.open(QIODevice::WriteOnly);
    exportOutput.close();

    // Try exporting an attachment of a non-existent entry
    setInput("a");
    execCmd(attachmentExportCmd,
            {"attachment-export",
             m_dbFile->fileName(),
             "invalid_entry_path",
             "invalid_attachment_name",
             exportOutput.fileName()});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Could not find entry with path invalid_entry_path.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Try exporting a non-existent attachment
    setInput("a");
    execCmd(attachmentExportCmd,
            {"attachment-export",
             m_dbFile->fileName(),
             "/Sample Entry",
             "invalid_attachment_name",
             exportOutput.fileName()});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Could not find attachment with name invalid_attachment_name.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Export an existing attachment to a file
    setInput("a");
    execCmd(
        attachmentExportCmd,
        {"attachment-export", m_dbFile->fileName(), "/Sample Entry", "Sample attachment.txt", exportOutput.fileName()});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray(qPrintable(QString("Successfully exported attachment %1 of entry %2 to %3.\n")
                                       .arg("Sample attachment.txt", "/Sample Entry", exportOutput.fileName()))));

    exportOutput.open(QIODevice::ReadOnly);
    QCOMPARE(exportOutput.readAll(), QByteArray("Sample content\n"));

    // Export an existing attachment to stdout
    setInput("a");
    execCmd(attachmentExportCmd,
            {"attachment-export", "--stdout", m_dbFile->fileName(), "/Sample Entry", "Sample attachment.txt"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Sample content\n"));

    // Ensure --stdout works even in quiet mode
    setInput("a");
    execCmd(
        attachmentExportCmd,
        {"attachment-export", "--quiet", "--stdout", m_dbFile->fileName(), "/Sample Entry", "Sample attachment.txt"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Sample content\n"));
}

void TestCli::testAttachmentImport()
{
    AttachmentImport attachmentImportCmd;
    QVERIFY(!attachmentImportCmd.name.isEmpty());
    QVERIFY(attachmentImportCmd.getDescriptionLine().contains(attachmentImportCmd.name));

    const QString attachmentPath = QString(KEEPASSX_TEST_DATA_DIR).append("/Attachment.txt");
    QVERIFY(QFile::exists(attachmentPath));

    // Try importing an attachment to a non-existent entry
    setInput("a");
    execCmd(attachmentImportCmd,
            {"attachment-import",
             m_dbFile->fileName(),
             "invalid_entry_path",
             "invalid_attachment_name",
             "invalid_attachment_path"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Could not find entry with path invalid_entry_path.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Try importing an attachment with an occupied name without -f option
    setInput("a");
    execCmd(attachmentImportCmd,
            {"attachment-import",
             m_dbFile->fileName(),
             "/Sample Entry",
             "Sample attachment.txt",
             "invalid_attachment_path"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(),
             QByteArray("Attachment Sample attachment.txt already exists for entry /Sample Entry.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Try importing a non-existent attachment
    setInput("a");
    execCmd(attachmentImportCmd,
            {"attachment-import",
             m_dbFile->fileName(),
             "/Sample Entry",
             "Sample attachment 2.txt",
             "invalid_attachment_path"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Could not open attachment file invalid_attachment_path.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Try importing an attachment with an occupied name with -f option
    setInput("a");
    execCmd(
        attachmentImportCmd,
        {"attachment-import", "-f", m_dbFile->fileName(), "/Sample Entry", "Sample attachment.txt", attachmentPath});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray(qPrintable(
                 QString("Successfully imported attachment %1 as Sample attachment.txt to entry /Sample Entry.\n")
                     .arg(attachmentPath))));

    // Try importing an attachment with an unoccupied name
    setInput("a");
    execCmd(attachmentImportCmd,
            {"attachment-import", m_dbFile->fileName(), "/Sample Entry", "Attachment.txt", attachmentPath});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(
        m_stdout->readAll(),
        QByteArray(qPrintable(QString("Successfully imported attachment %1 as Attachment.txt to entry /Sample Entry.\n")
                                  .arg(attachmentPath))));
}

void TestCli::testAttachmentRemove()
{
    AttachmentRemove attachmentRemoveCmd;
    QVERIFY(!attachmentRemoveCmd.name.isEmpty());
    QVERIFY(attachmentRemoveCmd.getDescriptionLine().contains(attachmentRemoveCmd.name));

    // Try deleting an attachment belonging to a non-existent entry
    setInput("a");
    execCmd(attachmentRemoveCmd,
            {"attachment-rm", m_dbFile->fileName(), "invalid_entry_path", "invalid_attachment_name"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Could not find entry with path invalid_entry_path.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Try deleting a non-existent attachment from an entry
    setInput("a");
    execCmd(attachmentRemoveCmd, {"attachment-rm", m_dbFile->fileName(), "/Sample Entry", "invalid_attachment_name"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Could not find attachment with name invalid_attachment_name.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Finally delete an existing attachment from an existing entry
    auto db = readDatabase();
    QVERIFY(db);

    const Entry* entry = db->rootGroup()->findEntryByPath("/Sample Entry");
    QVERIFY(entry);

    QVERIFY(entry->attachments()->hasKey("Sample attachment.txt"));

    setInput("a");
    execCmd(attachmentRemoveCmd, {"attachment-rm", m_dbFile->fileName(), "/Sample Entry", "Sample attachment.txt"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Successfully removed attachment Sample attachment.txt from entry /Sample Entry.\n"));

    db = readDatabase();
    QVERIFY(db);
    QVERIFY(!db->rootGroup()->findEntryByPath("/Sample Entry")->attachments()->hasKey("Sample attachment.txt"));
}

void TestCli::testClip()
{
    if (QProcessEnvironment::systemEnvironment().contains("WAYLAND_DISPLAY")) {
        QSKIP("Clip test skipped due to QClipboard and Wayland issues on Linux");
    }

    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->clear();

    Clip clipCmd;
    QVERIFY(!clipCmd.name.isEmpty());
    QVERIFY(clipCmd.getDescriptionLine().contains(clipCmd.name));

    // Password
    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFile->fileName(), "/Sample Entry", "0"});
    QString errorOutput(m_stderr->readAll());

    if (errorOutput.contains("Unable to start program")
        || errorOutput.contains("No program defined for clipboard manipulation")) {
        QSKIP("Clip test skipped due to missing clipboard tool");
    }
    QVERIFY(!errorOutput.contains("All clipping programs failed"));

    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QTRY_COMPARE(clipboard->text(), QString("Password"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Entry's \"Password\" attribute copied to the clipboard!\n"));

    // Quiet option
    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFile->fileName(), "/Sample Entry", "0", "-q"});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QTRY_COMPARE(clipboard->text(), QString("Password"));

    // Username
    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFile->fileName(), "/Sample Entry", "0", "-a", "username"});
    QTRY_COMPARE(clipboard->text(), QString("User Name"));

    // Uuid (top-level field)
    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFile->fileName(), "/Sample Entry", "0", "-a", "Uuid"});
    QTRY_COMPARE(clipboard->text(), QString("{9f4544c2-ab00-c74a-8a1a-6eaf26cf57e9}"));

    // TOTP
    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFile->fileName(), "/Sample Entry", "0", "--totp"});
    QTRY_VERIFY(isTotp(clipboard->text()));
    QCOMPARE(m_stdout->readLine(), QByteArray("Entry's \"totp\" attribute copied to the clipboard!\n"));

    // Test Unicode
    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFile2->fileName(), "/General/Unicode", "0", "-a", "username"});
    QTRY_COMPARE(clipboard->text(), QString(R"(¯\_(ツ)_/¯)"));

    // Password with timeout
    setInput("a");
    // clang-format off
    auto future = QtConcurrent::run(static_cast<int(Clip::*)(const QStringList&)>(&DatabaseCommand::execute),
                                &clipCmd,
                                QStringList{"clip", m_dbFile->fileName(), "/Sample Entry", "1"});
    // clang-format on

    QTRY_COMPARE(clipboard->text(), QString("Password"));
    QTRY_COMPARE_WITH_TIMEOUT(clipboard->text(), QString(""), 3000);

    future.waitForFinished();

    // TOTP with timeout
    setInput("a");
    future = QtConcurrent::run(static_cast<int (Clip::*)(const QStringList&)>(&DatabaseCommand::execute),
                               &clipCmd,
                               QStringList{"clip", m_dbFile->fileName(), "/Sample Entry", "1", "-t"});

    QTRY_VERIFY(isTotp(clipboard->text()));
    QTRY_COMPARE_WITH_TIMEOUT(clipboard->text(), QString(""), 3000);

    future.waitForFinished();

    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFile->fileName(), "--totp", "/Sample Entry", "bleuh"});
    QVERIFY(m_stderr->readAll().contains("Invalid timeout value bleuh.\n"));

    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFile2->fileName(), "--totp", "/Sample Entry", "0"});
    QVERIFY(m_stderr->readAll().contains("Entry with path /Sample Entry has no TOTP set up.\n"));

    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFile->fileName(), "-a", "TESTAttribute1", "/Sample Entry", "0"});
    QVERIFY(m_stderr->readAll().contains("ERROR: attribute TESTAttribute1 is ambiguous"));

    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFile2->fileName(), "--attribute", "Username", "--totp", "/Sample Entry", "0"});
    QVERIFY(m_stderr->readAll().contains("ERROR: Please specify one of --attribute or --totp, not both.\n"));

    // Best option
    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFileMulti->fileName(), "Multi", "0", "-b"});
    QByteArray errorChoices = m_stderr->readAll();
    QVERIFY(errorChoices.contains("Multi Entry 1"));
    QVERIFY(errorChoices.contains("Multi Entry 2"));

    setInput("a");
    execCmd(clipCmd, {"clip", m_dbFileMulti->fileName(), "Entry 2", "0", "-b"});
    QTRY_COMPARE(clipboard->text(), QString("Password2"));
}

void TestCli::testCreate()
{
    DatabaseCreate createCmd;
    QVERIFY(!createCmd.name.isEmpty());
    QVERIFY(createCmd.getDescriptionLine().contains(createCmd.name));

    QScopedPointer<QTemporaryDir> testDir(new QTemporaryDir());
    QString dbFilename;

    // Testing password option, password mismatch
    dbFilename = testDir->path() + "/testCreate_pw.kdbx";
    setInput({"a", "b"});
    execCmd(createCmd, {"db-create", dbFilename, "-p"});

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Repeat password: \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Error: Passwords do not match.\n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Failed to set database password.\n"));

    // Testing password option
    setInput({"a", "a"});
    execCmd(createCmd, {"db-create", dbFilename, "-p"});

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Repeat password: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Successfully created new database.\n"));

    auto db = readDatabase(dbFilename, "a");
    QVERIFY(db);

    // Testing with empty password (deny it)
    dbFilename = testDir->path() + "/testCreate_blankpw.kdbx";
    setInput({"", "n"});
    execCmd(createCmd, {"db-create", dbFilename, "-p"});

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QVERIFY(m_stderr->readLine().contains("empty password"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Failed to set database password.\n"));

    // Testing with empty password (accept it)
    setInput({"", "y"});
    execCmd(createCmd, {"db-create", dbFilename, "-p"});

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QVERIFY(m_stderr->readLine().contains("empty password"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Successfully created new database.\n"));

    db = readDatabase(dbFilename, "");
    QVERIFY(db);

    // Should refuse to create the database if it already exists.
    execCmd(createCmd, {"db-create", dbFilename, "-p"});
    // Output should be empty when there is an error.
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QString errorMessage = QString("File " + dbFilename + " already exists.\n");
    QCOMPARE(m_stderr->readAll(), errorMessage.toUtf8());

    // Should refuse to create without any key provided.
    dbFilename = testDir->path() + "/testCreate_key.kdbx";
    execCmd(createCmd, {"db-create", dbFilename});
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QCOMPARE(m_stderr->readLine(), QByteArray("No key is set. Aborting database creation.\n"));

    // Testing with keyfile creation
    dbFilename = testDir->path() + "/testCreate_key2.kdbx";
    QString keyfilePath = testDir->path() + "/keyfile.txt";
    setInput({"a", "a"});
    execCmd(createCmd, {"db-create", dbFilename, "-p", "-k", keyfilePath});

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Repeat password: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Successfully created new database.\n"));

    db = readDatabase(dbFilename, "a", keyfilePath);
    QVERIFY(db);

    // Testing with existing keyfile
    dbFilename = testDir->path() + "/testCreate_key3.kdbx";
    setInput({"a", "a"});
    execCmd(createCmd, {"db-create", dbFilename, "-p", "-k", keyfilePath});

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Repeat password: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Successfully created new database.\n"));

    db = readDatabase(dbFilename, "a", keyfilePath);
    QVERIFY(db);

    // Invalid decryption time (format).
    dbFilename = testDir->path() + "/testCreate_time.kdbx";
    execCmd(createCmd, {"db-create", dbFilename, "-p", "-t", "NAN"});

    QCOMPARE(m_stdout->readAll(), QByteArray());
    QCOMPARE(m_stderr->readAll(), QByteArray("Invalid decryption time NAN.\n"));

    // Invalid decryption time (range).
    execCmd(createCmd, {"db-create", dbFilename, "-p", "-t", "10"});

    QCOMPARE(m_stdout->readAll(), QByteArray());
    QVERIFY(m_stderr->readAll().contains(QByteArray("Target decryption time must be between")));

    int encryptionTime = 500;
    // Custom encryption time
    setInput({"a", "a"});
    int epochBefore = QDateTime::currentMSecsSinceEpoch();
    execCmd(createCmd, {"db-create", dbFilename, "-p", "-t", QString::number(encryptionTime)});
    // Removing 100ms to make sure we account for changes in computation time.
    QVERIFY(QDateTime::currentMSecsSinceEpoch() > (epochBefore + encryptionTime - 100));

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Repeat password: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Benchmarking key derivation function for 500ms delay.\n"));
    QVERIFY(m_stdout->readLine().contains(QByteArray("rounds for key derivation function.\n")));

    db = readDatabase(dbFilename, "a");
    QVERIFY(db);
}

void TestCli::testDatabaseEdit()
{
    TemporaryFile firstKeyFile;
    firstKeyFile.open();
    firstKeyFile.write(QString("keyFilePassword").toLatin1());
    firstKeyFile.close();

    TemporaryFile secondKeyFile;
    secondKeyFile.open();
    secondKeyFile.write(QString("newKeyFilePassword").toLatin1());
    secondKeyFile.close();

    QScopedPointer<QTemporaryDir> testDir(new QTemporaryDir());

    DatabaseCreate createCmd;
    DatabaseEdit editCmd;
    QVERIFY(!editCmd.name.isEmpty());
    QVERIFY(editCmd.getDescriptionLine().contains(editCmd.name));

    QString dbFilename;
    dbFilename = testDir->path() + "/testDatabaseEdit.kdbx";

    // Creating a database for testing
    setInput({"a", "a"});
    execCmd(createCmd, {"db-create", dbFilename, "-p"});
    QCOMPARE(m_stdout->readLine(), QByteArray("Successfully created new database.\n"));

    // Sanity check.
    auto db = readDatabase(dbFilename, "a");
    QVERIFY(!db.isNull());

    setInput("a");
    execCmd(editCmd, {"db-edit", dbFilename, "-p", "--unset-password"});
    QCOMPARE(m_stdout->readAll(), QByteArray(""));
    m_stderr->readLine();
    QCOMPARE(m_stderr->readAll(), QByteArray("Cannot use p and unset-password at the same time.\n"));

    setInput("a");
    execCmd(editCmd, {"db-edit", dbFilename, "--set-key-file", "/key/file/path", "--unset-key-file"});
    QCOMPARE(m_stdout->readAll(), QByteArray(""));
    // Skipping the password prompt.
    m_stderr->readLine();
    QCOMPARE(m_stderr->readAll(), QByteArray("Cannot use set-key-file and unset-key-file at the same time.\n"));

    // Sanity check.
    db = readDatabase(dbFilename, "a");
    QVERIFY(!db.isNull());

    setInput({"a", "b", "b"});
    execCmd(editCmd, {"db-edit", dbFilename, "-p"});
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully edited the database.\n"));

    // Sanity check
    db = readDatabase(dbFilename, "b");
    QVERIFY(!db.isNull());

    setInput("b");
    execCmd(editCmd, {"db-edit", dbFilename, "--set-key-file", firstKeyFile.fileName()});
    // Skipping the password prompt.
    m_stderr->readLine();
    QCOMPARE(m_stderr->readAll(), QByteArray(""));
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully edited the database.\n"));

    // Sanity check
    db = readDatabase(dbFilename, "b");
    QVERIFY(db.isNull());
    db = readDatabase(dbFilename, "b", firstKeyFile.fileName());
    QVERIFY(!db.isNull());

    setInput("b");
    execCmd(editCmd,
            {"db-edit", dbFilename, "-k", firstKeyFile.fileName(), "--set-key-file", secondKeyFile.fileName()});
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully edited the database.\n"));

    // Sanity check
    db = readDatabase(dbFilename, "b", firstKeyFile.fileName());
    QVERIFY(db.isNull());
    db = readDatabase(dbFilename, "b", secondKeyFile.fileName());
    QVERIFY(!db.isNull());

    setInput("b");
    execCmd(editCmd, {"db-edit", dbFilename, "-k", secondKeyFile.fileName(), "--unset-password"});
    // Skipping the password prompt.
    m_stderr->readLine();
    QCOMPARE(m_stderr->readAll(), QByteArray(""));
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully edited the database.\n"));

    execCmd(editCmd,
            {"db-edit",
             dbFilename,
             "--no-password",
             "-k",
             secondKeyFile.fileName(),
             "--set-key-file",
             firstKeyFile.fileName()});
    // Skipping the password prompt.
    m_stderr->readLine();
    QCOMPARE(m_stderr->readAll(), QByteArray(""));
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully edited the database.\n"));

    setInput({"b", "b"});
    execCmd(editCmd, {"db-edit", dbFilename, "-k", firstKeyFile.fileName(), "--no-password", "--set-password"});
    // Skipping over the password setting prompts.
    m_stderr->readLine();
    m_stderr->readLine();
    QCOMPARE(m_stderr->readAll(), QByteArray(""));
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully edited the database.\n"));

    setInput("b");
    execCmd(editCmd, {"db-edit", dbFilename, "-k", firstKeyFile.fileName(), "--unset-key-file"});
    // Skipping the password prompt.
    m_stderr->readLine();
    QCOMPARE(m_stderr->readAll(), QByteArray(""));
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully edited the database.\n"));

    // Sanity check
    db = readDatabase(dbFilename, "b", firstKeyFile.fileName());
    QVERIFY(db.isNull());
    db = readDatabase(dbFilename, "b");
    QVERIFY(!db.isNull());

    // Trying to remove the key file when there is none set should
    // raise an error.
    setInput("b");
    execCmd(editCmd, {"db-edit", dbFilename, "-p", "--unset-key-file"});
    QCOMPARE(m_stdout->readAll(), QByteArray(""));
    m_stderr->readLine();
    QCOMPARE(m_stderr->readLine(), QByteArray("Cannot remove file key: The database does not have a file key.\n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Could not change the database key.\n"));

    setInput("b");
    execCmd(editCmd, {"db-edit", dbFilename, "--unset-password"});
    QCOMPARE(m_stdout->readAll(), QByteArray(""));
    // Skipping the password prompt.
    m_stderr->readLine();
    QCOMPARE(m_stderr->readLine(), QByteArray("Cannot remove all the keys from a database.\n"));
}

void TestCli::testInfo()
{
    DatabaseInfo infoCmd;
    QVERIFY(!infoCmd.name.isEmpty());
    QVERIFY(infoCmd.getDescriptionLine().contains(infoCmd.name));

    setInput("a");
    execCmd(infoCmd, {"db-info", m_dbFile->fileName()});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QVERIFY(m_stdout->readLine().contains(QByteArray("UUID: ")));
    QCOMPARE(m_stdout->readLine(), QByteArray("Name: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Description: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Cipher: AES 256-bit\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("KDF: AES (6000 rounds)\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Recycle bin is enabled.\n"));
    QVERIFY(m_stdout->readLine().contains(m_dbFile->fileName().toUtf8()));
    QVERIFY(m_stdout->readLine().contains(
        QByteArray("Database created: "))); // date changes often, so just test for the first part
    QVERIFY(m_stdout->readLine().contains(
        QByteArray("Last saved: "))); // date changes often, so just test for the first part
    QCOMPARE(m_stdout->readLine(), QByteArray("Unsaved changes: no\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Number of groups: 8\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Number of entries: 2\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Number of expired entries: 0\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Unique passwords: 2\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Non-unique passwords: 0\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Maximum password reuse: 1\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Number of short passwords: 0\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Number of weak passwords: 2\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Entries excluded from reports: 0\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Average password length: 11 character(s)\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Number of passkeys: 0\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Entries with TOTP setup: 1\n"));

    // Test with quiet option.
    setInput("a");
    execCmd(infoCmd, {"db-info", "-q", m_dbFile->fileName()});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QVERIFY(m_stdout->readLine().contains(QByteArray("UUID: ")));
    QCOMPARE(m_stdout->readLine(), QByteArray("Name: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Description: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Cipher: AES 256-bit\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("KDF: AES (6000 rounds)\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Recycle bin is enabled.\n"));
}

void TestCli::testDiceware()
{
    Diceware dicewareCmd;
    QVERIFY(!dicewareCmd.name.isEmpty());
    QVERIFY(dicewareCmd.getDescriptionLine().contains(dicewareCmd.name));

    execCmd(dicewareCmd, {"diceware"});
    QString passphrase(m_stdout->readLine());
    QVERIFY(!passphrase.isEmpty());

    execCmd(dicewareCmd, {"diceware", "-W", "2"});
    passphrase = m_stdout->readLine();
    QCOMPARE(passphrase.split(" ").size(), 2);

    execCmd(dicewareCmd, {"diceware", "-W", "10"});
    passphrase = m_stdout->readLine();
    QCOMPARE(passphrase.split(" ").size(), 10);

    // Testing with invalid word count
    execCmd(dicewareCmd, {"diceware", "-W", "-10"});
    QCOMPARE(m_stderr->readLine(), QByteArray("Invalid word count -10\n"));

    // Testing with invalid word count format
    execCmd(dicewareCmd, {"diceware", "-W", "bleuh"});
    QCOMPARE(m_stderr->readLine(), QByteArray("Invalid word count bleuh\n"));

    TemporaryFile wordFile;
    wordFile.open();
    for (int i = 0; i < 4500; ++i) {
        wordFile.write(QString("word" + QString::number(i) + "\n").toLatin1());
    }
    wordFile.close();

    execCmd(dicewareCmd, {"diceware", "-W", "11", "-w", wordFile.fileName()});
    passphrase = m_stdout->readLine();
    const auto words = passphrase.split(" ");
    QCOMPARE(words.size(), 11);
    QRegularExpression regex("^word\\d+$");
    for (const auto& word : words) {
        QVERIFY2(regex.match(word).hasMatch(), qPrintable("Word " + word + " was not on the word list"));
    }

    TemporaryFile smallWordFile;
    smallWordFile.open();
    for (int i = 0; i < 50; ++i) {
        smallWordFile.write(QString("word" + QString::number(i) + "\n").toLatin1());
    }
    smallWordFile.close();

    // Ensure a warning is shown if the wordlist is too short
    execCmd(dicewareCmd, {"diceware", "-W", "11", "-w", smallWordFile.fileName()});
    QVERIFY(m_stderr->readLine().length() > 0);
}

void TestCli::testEdit()
{
    Edit editCmd;
    QVERIFY(!editCmd.name.isEmpty());
    QVERIFY(editCmd.getDescriptionLine().contains(editCmd.name));

    setInput("a");
    execCmd(editCmd,
            {"edit",
             "-u",
             "newuser",
             "--url",
             "https://otherurl.example.com/",
             "--notes",
             "newnotes",
             "-t",
             "newtitle",
             m_dbFile->fileName(),
             "/Sample Entry"});
    QCOMPARE(m_stdout->readLine(), QByteArray("Successfully edited entry newtitle.\n"));

    auto db = readDatabase();
    auto* entry = db->rootGroup()->findEntryByPath("/newtitle");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://otherurl.example.com/"));
    QCOMPARE(entry->password(), QString("Password"));
    QCOMPARE(entry->notes(), QString("newnotes"));

    // Quiet option
    setInput("a");
    execCmd(editCmd, {"edit", m_dbFile->fileName(), "-q", "-t", "newertitle", "/newtitle"});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray());

    setInput("a");
    execCmd(editCmd, {"edit", "-g", m_dbFile->fileName(), "/newertitle"});
    db = readDatabase();
    entry = db->rootGroup()->findEntryByPath("/newertitle");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://otherurl.example.com/"));
    QVERIFY(!entry->password().isEmpty());
    QVERIFY(entry->password() != QString("Password"));

    setInput("a");
    execCmd(editCmd, {"edit", "-g", "-L", "34", "-t", "evennewertitle", m_dbFile->fileName(), "/newertitle"});
    db = readDatabase();
    entry = db->rootGroup()->findEntryByPath("/evennewertitle");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://otherurl.example.com/"));
    QVERIFY(entry->password() != QString("Password"));
    QCOMPARE(entry->password().size(), 34);
    QRegularExpression defaultPasswordClassesRegex("^[a-zA-Z0-9]+$");
    QVERIFY(defaultPasswordClassesRegex.match(entry->password()).hasMatch());

    setInput("a");
    execCmd(editCmd,
            {"edit",
             "-g",
             "-L",
             "20",
             "--every-group",
             "-s",
             "-n",
             "--upper",
             "-l",
             m_dbFile->fileName(),
             "/evennewertitle"});
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully edited entry evennewertitle.\n"));

    db = readDatabase();
    entry = db->rootGroup()->findEntryByPath("/evennewertitle");
    QVERIFY(entry);
    QCOMPARE(entry->password().size(), 20);
    QVERIFY(!defaultPasswordClassesRegex.match(entry->password()).hasMatch());

    setInput({"a", "newpassword"});
    execCmd(editCmd, {"edit", "-p", m_dbFile->fileName(), "/evennewertitle"});
    db = readDatabase();
    QVERIFY(db);
    entry = db->rootGroup()->findEntryByPath("/evennewertitle");
    QVERIFY(entry);
    QCOMPARE(entry->password(), QString("newpassword"));

    // with line break in notes
    setInput("a");
    execCmd(editCmd, {"edit", m_dbFile->fileName(), "--notes", "testing\\nline breaks", "/evennewertitle"});
    db = readDatabase();
    entry = db->rootGroup()->findEntryByPath("/evennewertitle");
    QVERIFY(entry);
    QCOMPARE(entry->notes(), QString("testing\nline breaks"));
}

void TestCli::testEstimate_data()
{
    // clang-format off
    QTest::addColumn<QString>("input");
    QTest::addColumn<QStringList>("searchStrings");

    QTest::newRow("Dictionary")
        << "password"
        << QStringList{"Type: Dictionary", "\tpassword"};

    QTest::newRow("Spatial")
        << "sdfg"
        << QStringList{"Type: Spatial", "\tsdfg"};

    QTest::newRow("Spatial(Rep)")
        << "sdfgsdfg"
        << QStringList{"Type: Spatial(Rep)", "\tsdfgsdfg"};

    QTest::newRow("Dictionary / Sequence")
        << "password123"
        << QStringList{"Type: Dictionary", "Type: Sequence", "\tpassword", "\t123"};

    QTest::newRow("Dict+Leet")
        << "p455w0rd"
        << QStringList{"Type: Dict+Leet", "\tp455w0rd"};

    QTest::newRow("Dictionary(Rep)")
        << "hellohello"
        << QStringList{"Type: Dictionary(Rep)", "\thellohello"};

    QTest::newRow("Sequence(Rep) / Dictionary")
        << "456456foobar"
        << QStringList{"Type: Sequence(Rep)", "Type: Dictionary", "\t456456", "\tfoobar"};

    QTest::newRow("Bruteforce(Rep) / Bruteforce")
        << "xzxzy"
        << QStringList{"Type: Bruteforce(Rep)", "Type: Bruteforce", "\txzxz", "\ty"};

    QTest::newRow("Dictionary / Date(Rep)")
        << "pass20182018"
        << QStringList{"Type: Dictionary", "Type: Date(Rep)", "\tpass", "\t20182018"};

    QTest::newRow("Dictionary / Date / Bruteforce")
        << "mypass2018-2"
        << QStringList{"Type: Dictionary", "Type: Date", "Type: Bruteforce", "\tmypass", "\t2018", "\t-2"};

    QTest::newRow("Strong Password")
        << "E*!%.Qw{t.X,&bafw)\"Q!ah$%;U/"
        << QStringList{"Type: Bruteforce", "\tE*"};

    // TODO: detect passphrases and adjust entropy calculation accordingly (issue #2347)
    QTest::newRow("Strong Passphrase")
        << "squint wooing resupply dangle isolation axis headsman"
        << QStringList{"Type: Dictionary", "Type: Bruteforce", "Multi-word extra bits 22.0", "\tsquint", "\t ", "\twooing"};
    // clang-format on
}

void TestCli::testEstimate()
{
    QFETCH(QString, input);
    QFETCH(QStringList, searchStrings);

    // Calculate expected values since zxcvbn output can vary by platform if different wordlists are used
    const auto e = ZxcvbnMatch(input.toUtf8(), nullptr, nullptr);
    auto length = QString::number(input.length());
    auto entropy = QString("%1").arg(e, 0, 'f', 3);
    auto log10 = QString("%1").arg(e * 0.301029996, 0, 'f', 3);

    Estimate estimateCmd;
    QVERIFY(!estimateCmd.name.isEmpty());
    QVERIFY(estimateCmd.getDescriptionLine().contains(estimateCmd.name));

    setInput(input);
    execCmd(estimateCmd, {"estimate", "-a"});
    auto result = QString(m_stdout->readAll());
    QVERIFY(result.contains("Length " + length));
    QVERIFY(result.contains("Entropy " + entropy));
    QVERIFY(result.contains("Log10 " + log10));
    for (const auto& string : asConst(searchStrings)) {
        QVERIFY2(result.contains(string), qPrintable("String " + string + " missing"));
    }
}

void TestCli::testExport()
{
    Export exportCmd;
    QVERIFY(!exportCmd.name.isEmpty());
    QVERIFY(exportCmd.getDescriptionLine().contains(exportCmd.name));

    setInput("a");
    execCmd(exportCmd, {"export", m_dbFile->fileName()});

    TemporaryFile xmlOutput;
    xmlOutput.open(QIODevice::WriteOnly);
    xmlOutput.write(m_stdout->readAll());
    xmlOutput.close();

    QScopedPointer<Database> db(new Database());
    QVERIFY(db->import(xmlOutput.fileName()));

    auto* entry = db->rootGroup()->findEntryByPath("/Sample Entry");
    QVERIFY(entry);
    QCOMPARE(entry->password(), QString("Password"));

    // Quiet option
    QScopedPointer<Database> dbQuiet(new Database());
    setInput("a");
    execCmd(exportCmd, {"export", "-f", "xml", "-q", m_dbFile->fileName()});
    QCOMPARE(m_stderr->readAll(), QByteArray());

    xmlOutput.open(QIODevice::WriteOnly);
    xmlOutput.write(m_stdout->readAll());
    xmlOutput.close();

    QVERIFY(db->import(xmlOutput.fileName()));

    // CSV exporting
    setInput("a");
    execCmd(exportCmd, {"export", "-f", "csv", m_dbFile->fileName()});
    QByteArray csvHeader = m_stdout->readLine();
    QVERIFY(csvHeader.contains(QByteArray("\"Group\",\"Title\",\"Username\",\"Password\",\"URL\",\"Notes\"")));
    QByteArray csvData = m_stdout->readAll();
    QVERIFY(csvData.contains(QByteArray(
        "\"NewDatabase\",\"Sample Entry\",\"User Name\",\"Password\",\"http://www.somesite.com/\",\"Notes\"")));

    // HTML exporting
    setInput("a");
    execCmd(exportCmd, {"export", "-f", "html", m_dbFile->fileName()});
    QByteArray htmlHeader = m_stdout->readLine();
    QVERIFY(htmlHeader.contains(QByteArray("<meta charset=\"UTF-8\"><title></title>")));
    QByteArray htmlBody = m_stdout->readAll();
    QVERIFY(htmlBody.contains(QByteArray("<h2>NewDatabase</h2>")));
    QVERIFY(htmlBody.contains(QByteArray("<caption>Sample Entry</caption>"
                                         "<tr><th>User name</th><td class=\"username\">User Name</td></tr>"
                                         "<tr><th>Password</th><td class=\"password\">Password</td></tr>"
                                         "<tr><th>URL</th><td class=\"url\"><a "
                                         "href=\"http://www.somesite.com/\">http://www.somesite.com/</a></td></tr>")));
    // test invalid format
    setInput("a");
    execCmd(exportCmd, {"export", "-f", "yaml", m_dbFile->fileName()});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readLine(), QByteArray("Unsupported format yaml\n"));
}

void TestCli::testGenerate_data()
{
    QTest::addColumn<QStringList>("parameters");
    QTest::addColumn<QString>("pattern");

    QTest::newRow("default") << QStringList{"generate"} << "^[^\r\n]+$";
    QTest::newRow("length") << QStringList{"generate", "-L", "13"} << "^.{13}$";
    QTest::newRow("lowercase") << QStringList{"generate", "-L", "14", "-l"} << "^[a-z]{14}$";
    QTest::newRow("uppercase") << QStringList{"generate", "-L", "15", "--upper"} << "^[A-Z]{15}$";
    QTest::newRow("numbers") << QStringList{"generate", "-L", "16", "-n"} << "^[0-9]{16}$";
    QTest::newRow("special") << QStringList{"generate", "-L", "200", "-s"}
                             << R"(^[\(\)\[\]\{\}\.\-*|\\,:;"'\/\_!+-<=>?#$%&^`@~]{200}$)";
    QTest::newRow("special (exclude)") << QStringList{"generate", "-L", "200", "-s", "-x", "+.?@&"}
                                       << R"(^[\(\)\[\]\{\}\.\-*|\\,:;"'\/\_!-<=>#$%^`~]{200}$)";
    QTest::newRow("extended") << QStringList{"generate", "-L", "50", "-e"}
                              << R"(^[^a-zA-Z0-9\(\)\[\]\{\}\.\-\*\|\\,:;"'\/\_!+-<=>?#$%&^`@~]{50}$)";
    QTest::newRow("numbers + lowercase + uppercase")
        << QStringList{"generate", "-L", "16", "-n", "--upper", "-l"} << "^[0-9a-zA-Z]{16}$";
    QTest::newRow("numbers + lowercase + uppercase (exclude)")
        << QStringList{"generate", "-L", "500", "-n", "-U", "-l", "-x", "abcdefg0123@"} << "^[^abcdefg0123@]{500}$";
    QTest::newRow("numbers + lowercase + uppercase (exclude similar)")
        << QStringList{"generate", "-L", "200", "-n", "-U", "-l", "--exclude-similar"} << "^[^l1IO0]{200}$";
    QTest::newRow("uppercase + lowercase (every)")
        << QStringList{"generate", "-L", "2", "--upper", "-l", "--every-group"} << "^[a-z][A-Z]|[A-Z][a-z]$";
    QTest::newRow("numbers + lowercase (every)")
        << QStringList{"generate", "-L", "2", "-n", "-l", "--every-group"} << "^[a-z][0-9]|[0-9][a-z]$";
    QTest::newRow("custom character set")
        << QStringList{"generate", "-L", "200", "-n", "-c", "abc"} << "^[abc0-9]{200}$";
    QTest::newRow("custom character set without extra options uses only custom chars")
        << QStringList{"generate", "-L", "200", "-c", "a"} << "^a{200}$";
}

void TestCli::testGenerate()
{
    QFETCH(QStringList, parameters);
    QFETCH(QString, pattern);

    Generate generateCmd;
    QVERIFY(!generateCmd.name.isEmpty());
    QVERIFY(generateCmd.getDescriptionLine().contains(generateCmd.name));

    for (int i = 0; i < 10; ++i) {
        execCmd(generateCmd, parameters);
        QRegularExpression regex(pattern);

        QString password = QString::fromUtf8(m_stdout->readLine());

        QVERIFY2(regex.match(password).hasMatch(),
                 qPrintable("Password " + password + " does not match pattern " + pattern));
        QCOMPARE(m_stderr->readAll(), QByteArray());
    }

    // Testing with invalid password length
    execCmd(generateCmd, {"generate", "-L", "-10"});
    QCOMPARE(m_stderr->readLine(), QByteArray("Invalid password length -10\n"));

    execCmd(generateCmd, {"generate", "-L", "0"});
    QCOMPARE(m_stderr->readLine(), QByteArray("Invalid password length 0\n"));

    // Testing with invalid word count format
    execCmd(generateCmd, {"generate", "-L", "bleuh"});
    QCOMPARE(m_stderr->readLine(), QByteArray("Invalid password length bleuh\n"));
}

void TestCli::testImport()
{
    Import importCmd;
    QVERIFY(!importCmd.name.isEmpty());
    QVERIFY(importCmd.getDescriptionLine().contains(importCmd.name));

    QScopedPointer<QTemporaryDir> testDir(new QTemporaryDir());
    QString databaseFilename = testDir->path() + "/testImport1.kdbx";

    setInput({"a", "a"});
    execCmd(importCmd, {"import", m_xmlFile->fileName(), databaseFilename, "-p"});

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Repeat password: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Successfully imported database.\n"));

    auto db = readDatabase(databaseFilename, "a");
    QVERIFY(db);
    auto* entry = db->rootGroup()->findEntryByPath("/Sample Entry 1");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("User Name"));

    // Should refuse to create the database if it already exists.
    execCmd(importCmd, {"import", m_xmlFile->fileName(), databaseFilename});
    // Output should be empty when there is an error.
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QString errorMessage = QString("File " + databaseFilename + " already exists.\n");
    QCOMPARE(m_stderr->readAll(), errorMessage.toUtf8());

    // Testing import with non-existing keyfile
    databaseFilename = testDir->path() + "/testImport2.kdbx";
    QString keyfilePath = testDir->path() + "/keyfile.txt";
    setInput({"a", "a"});
    execCmd(importCmd, {"import", "-p", "-k", keyfilePath, m_xmlFile->fileName(), databaseFilename});

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Repeat password: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Successfully imported database.\n"));

    db = readDatabase(databaseFilename, "a", keyfilePath);
    QVERIFY(db);

    // Testing import with existing keyfile
    databaseFilename = testDir->path() + "/testImport3.kdbx";
    setInput({"a", "a"});
    execCmd(importCmd, {"import", "-p", "-k", keyfilePath, m_xmlFile->fileName(), databaseFilename});

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Repeat password: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Successfully imported database.\n"));

    db = readDatabase(databaseFilename, "a", keyfilePath);
    QVERIFY(db);

    // Invalid decryption time (format).
    databaseFilename = testDir->path() + "/testCreate_time.kdbx";
    execCmd(importCmd, {"import", "-p", "-t", "NAN", m_xmlFile->fileName(), databaseFilename});

    QCOMPARE(m_stdout->readAll(), QByteArray());
    QCOMPARE(m_stderr->readAll(), QByteArray("Invalid decryption time NAN.\n"));

    // Invalid decryption time (range).
    execCmd(importCmd, {"import", "-p", "-t", "10", m_xmlFile->fileName(), databaseFilename});

    QCOMPARE(m_stdout->readAll(), QByteArray());
    QVERIFY(m_stderr->readAll().contains(QByteArray("Target decryption time must be between")));

    int encryptionTime = 500;
    // Custom encryption time
    setInput({"a", "a"});
    int epochBefore = QDateTime::currentMSecsSinceEpoch();
    execCmd(importCmd,
            {"import", "-p", "-t", QString::number(encryptionTime), m_xmlFile->fileName(), databaseFilename});
    // Removing 100ms to make sure we account for changes in computation time.
    QVERIFY(QDateTime::currentMSecsSinceEpoch() > (epochBefore + encryptionTime - 100));

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Repeat password: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray("Benchmarking key derivation function for 500ms delay.\n"));
    QVERIFY(m_stdout->readLine().contains(QByteArray("rounds for key derivation function.\n")));

    db = readDatabase(databaseFilename, "a");
    QVERIFY(db);

    // Quiet option
    QScopedPointer<QTemporaryDir> testDirQuiet(new QTemporaryDir());
    QString databaseFilenameQuiet = testDirQuiet->path() + "/testImport2.kdbx";

    setInput({"a", "a"});
    execCmd(importCmd, {"import", "-p", "-q", m_xmlFile->fileName(), databaseFilenameQuiet});

    QCOMPARE(m_stderr->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stderr->readLine(), QByteArray("Repeat password: \n"));
    QCOMPARE(m_stdout->readLine(), QByteArray());

    db = readDatabase(databaseFilenameQuiet, "a");
    QVERIFY(db);
}

void TestCli::testKeyFileOption()
{
    List listCmd;

    QString keyFilePath(QString(KEEPASSX_TEST_DATA_DIR).append("/KeyFileProtected.key"));
    setInput("a");
    execCmd(listCmd, {"ls", "-k", keyFilePath, m_keyFileProtectedDbFile->fileName()});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray("entry1\n"
                        "entry2\n"));

    // Should raise an error with no key file.
    setInput("a");
    execCmd(listCmd, {"ls", m_keyFileProtectedDbFile->fileName()});
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QVERIFY(m_stderr->readAll().contains("Invalid credentials were provided"));

    // Should raise an error if key file path is invalid.
    setInput("a");
    execCmd(listCmd, {"ls", "-k", "invalidpath", m_keyFileProtectedDbFile->fileName()});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QCOMPARE(m_stderr->readAll().split(':').at(0), QByteArray("Failed to load key file invalidpath"));
}

void TestCli::testNoPasswordOption()
{
    List listCmd;

    QString keyFilePath(QString(KEEPASSX_TEST_DATA_DIR).append("/KeyFileProtectedNoPassword.key"));
    execCmd(listCmd, {"ls", "-k", keyFilePath, "--no-password", m_keyFileProtectedNoPasswordDbFile->fileName()});
    // Expecting no password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray("entry1\n"
                        "entry2\n"));

    // Should raise an error with no key file.
    execCmd(listCmd, {"ls", "--no-password", m_keyFileProtectedNoPasswordDbFile->fileName()});
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QVERIFY(m_stderr->readAll().contains("Invalid credentials were provided"));
}

void TestCli::testList()
{
    List listCmd;
    QVERIFY(!listCmd.name.isEmpty());
    QVERIFY(listCmd.getDescriptionLine().contains(listCmd.name));

    setInput("a");
    execCmd(listCmd, {"ls", m_dbFile->fileName()});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Sample Entry\n"
                        "General/\n"
                        "Windows/\n"
                        "Network/\n"
                        "Internet/\n"
                        "eMail/\n"
                        "Homebanking/\n"));

    // Quiet option
    setInput("a");
    execCmd(listCmd, {"ls", "-q", m_dbFile->fileName()});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Sample Entry\n"
                        "General/\n"
                        "Windows/\n"
                        "Network/\n"
                        "Internet/\n"
                        "eMail/\n"
                        "Homebanking/\n"));

    setInput("a");
    execCmd(listCmd, {"ls", "-R", m_dbFile->fileName()});
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Sample Entry\n"
                        "General/\n"
                        "  [empty]\n"
                        "Windows/\n"
                        "  [empty]\n"
                        "Network/\n"
                        "  [empty]\n"
                        "Internet/\n"
                        "  [empty]\n"
                        "eMail/\n"
                        "  [empty]\n"
                        "Homebanking/\n"
                        "  Subgroup/\n"
                        "    Subgroup Entry\n"));

    setInput("a");
    execCmd(listCmd, {"ls", "-R", "-f", m_dbFile->fileName()});
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Sample Entry\n"
                        "General/\n"
                        "General/[empty]\n"
                        "Windows/\n"
                        "Windows/[empty]\n"
                        "Network/\n"
                        "Network/[empty]\n"
                        "Internet/\n"
                        "Internet/[empty]\n"
                        "eMail/\n"
                        "eMail/[empty]\n"
                        "Homebanking/\n"
                        "Homebanking/Subgroup/\n"
                        "Homebanking/Subgroup/Subgroup Entry\n"));

    setInput("a");
    execCmd(listCmd, {"ls", "-R", "-f", m_dbFile->fileName(), "/Homebanking"});
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Subgroup/\n"
                        "Subgroup/Subgroup Entry\n"));

    setInput("a");
    execCmd(listCmd, {"ls", m_dbFile->fileName(), "/General/"});
    QCOMPARE(m_stdout->readAll(), QByteArray("[empty]\n"));

    setInput("a");
    execCmd(listCmd, {"ls", m_dbFile->fileName(), "/DoesNotExist/"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Cannot find group /DoesNotExist/.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());
}

void TestCli::testMerge()
{
    Merge mergeCmd;
    QVERIFY(!mergeCmd.name.isEmpty());
    QVERIFY(mergeCmd.getDescriptionLine().contains(mergeCmd.name));

    // load test database and save copies
    auto db = readDatabase();
    QVERIFY(db);
    TemporaryFile targetFile1;
    targetFile1.open();
    targetFile1.close();

    TemporaryFile targetFile2;
    targetFile2.open();
    targetFile2.close();

    TemporaryFile targetFile3;
    targetFile3.open();
    targetFile3.close();

    db->saveAs(targetFile1.fileName());
    db->saveAs(targetFile2.fileName());

    // save another copy with a different password
    auto oldKey = db->key();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("b"));
    db->setKey(key);
    db->saveAs(targetFile3.fileName());

    // Restore the original password
    db->setKey(oldKey);

    // then add a new entry to the in-memory database and save another copy
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setTitle("Some Website");
    entry->setPassword("secretsecretsecret");
    auto* group = db->rootGroup()->findGroupByPath("/Internet/");
    QVERIFY(group);
    group->addEntry(entry);

    TemporaryFile sourceFile;
    sourceFile.open();
    sourceFile.close();
    db->saveAs(sourceFile.fileName());

    setInput("a");
    execCmd(mergeCmd, {"merge", "-s", targetFile1.fileName(), sourceFile.fileName()});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QList<QByteArray> outLines1 = m_stdout->readAll().split('\n');
    QVERIFY(outLines1.at(0).contains("Modified"));
    QVERIFY(outLines1.at(0).contains("Modification time"));
    QVERIFY(outLines1.at(1).contains("Added"));
    QCOMPARE(outLines1.at(2),
             QString("Successfully merged %1 into %2.").arg(sourceFile.fileName(), targetFile1.fileName()).toUtf8());

    auto mergedDb = QSharedPointer<Database>::create();
    QVERIFY(mergedDb->open(targetFile1.fileName(), oldKey));

    auto* entry1 = mergedDb->rootGroup()->findEntryByPath("/Internet/Some Website");
    QVERIFY(entry1);
    QCOMPARE(entry1->title(), QString("Some Website"));
    QCOMPARE(entry1->password(), QString("secretsecretsecret"));

    // the dry run option should not modify the target database.
    setInput("a");
    execCmd(mergeCmd, {"merge", "--dry-run", "-s", targetFile2.fileName(), sourceFile.fileName()});
    QList<QByteArray> outLines2 = m_stdout->readAll().split('\n');
    QVERIFY(outLines1.at(0).contains("Modified"));
    QVERIFY(outLines1.at(0).contains("Modification time"));
    QVERIFY(outLines2.at(1).contains("Added"));
    QCOMPARE(outLines2.at(2), QByteArray("Database was not modified by merge operation."));

    mergedDb = QSharedPointer<Database>::create();
    QVERIFY(mergedDb->open(targetFile2.fileName(), oldKey));
    entry1 = mergedDb->rootGroup()->findEntryByPath("/Internet/Some Website");
    QVERIFY(!entry1);

    // the dry run option can be used with the quiet option
    setInput("a");
    execCmd(mergeCmd, {"merge", "--dry-run", "-s", "-q", targetFile2.fileName(), sourceFile.fileName()});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray());

    mergedDb = QSharedPointer<Database>::create();
    QVERIFY(mergedDb->open(targetFile2.fileName(), oldKey));
    entry1 = mergedDb->rootGroup()->findEntryByPath("/Internet/Some Website");
    QVERIFY(!entry1);

    // try again with different passwords for both files
    setInput({"b", "a"});
    execCmd(mergeCmd, {"merge", targetFile3.fileName(), sourceFile.fileName()});
    QList<QByteArray> outLines3 = m_stdout->readAll().split('\n');
    QCOMPARE(outLines3.at(2),
             QString("Successfully merged %1 into %2.").arg(sourceFile.fileName(), targetFile3.fileName()).toUtf8());

    mergedDb = QSharedPointer<Database>::create();
    QVERIFY(mergedDb->open(targetFile3.fileName(), key));

    entry1 = mergedDb->rootGroup()->findEntryByPath("/Internet/Some Website");
    QVERIFY(entry1);
    QCOMPARE(entry1->title(), QString("Some Website"));
    QCOMPARE(entry1->password(), QString("secretsecretsecret"));

    // making sure that the message is different if the database was not
    // modified by the merge operation.
    setInput("a");
    execCmd(mergeCmd, {"merge", "-s", sourceFile.fileName(), sourceFile.fileName()});
    QCOMPARE(m_stdout->readAll(), QByteArray("Database was not modified by merge operation.\n"));

    // Quiet option
    setInput("a");
    execCmd(mergeCmd, {"merge", "-q", "-s", sourceFile.fileName(), sourceFile.fileName()});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Quiet option without the -s option
    setInput({"a", "a"});
    execCmd(mergeCmd, {"merge", "-q", sourceFile.fileName(), sourceFile.fileName()});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray());
}

void TestCli::testMergeWithKeys()
{
    DatabaseCreate createCmd;
    QVERIFY(!createCmd.name.isEmpty());
    QVERIFY(createCmd.getDescriptionLine().contains(createCmd.name));

    Merge mergeCmd;
    QVERIFY(!mergeCmd.name.isEmpty());
    QVERIFY(mergeCmd.getDescriptionLine().contains(mergeCmd.name));

    QScopedPointer<QTemporaryDir> testDir(new QTemporaryDir());

    QString sourceDatabaseFilename = testDir->path() + "/testSourceDatabase.kdbx";
    QString sourceKeyfilePath = testDir->path() + "/testSourceKeyfile.txt";

    QString targetDatabaseFilename = testDir->path() + "/testTargetDatabase.kdbx";
    QString targetKeyfilePath = testDir->path() + "/testTargetKeyfile.txt";

    setInput({"a", "a"});
    execCmd(createCmd, {"db-create", sourceDatabaseFilename, "-p", "-k", sourceKeyfilePath});

    setInput({"b", "b"});
    execCmd(createCmd, {"db-create", targetDatabaseFilename, "-p", "-k", targetKeyfilePath});

    auto sourceDatabase = readDatabase(sourceDatabaseFilename, "a", sourceKeyfilePath);
    QVERIFY(sourceDatabase);

    auto targetDatabase = readDatabase(targetDatabaseFilename, "b", targetKeyfilePath);
    QVERIFY(targetDatabase);

    auto* rootGroup = new Group();
    rootGroup->setName("root");
    rootGroup->setUuid(QUuid::createUuid());
    auto* group = new Group();
    group->setUuid(QUuid::createUuid());
    group->setParent(rootGroup);
    group->setName("Internet");

    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setTitle("Some Website");
    entry->setPassword("secretsecretsecret");
    group->addEntry(entry);

    auto oldGroup = sourceDatabase->setRootGroup(rootGroup);
    delete oldGroup;

    auto* otherRootGroup = new Group();
    otherRootGroup->setName("root");
    otherRootGroup->setUuid(QUuid::createUuid());
    auto* otherGroup = new Group();
    otherGroup->setUuid(QUuid::createUuid());
    otherGroup->setParent(otherRootGroup);
    otherGroup->setName("Internet");

    auto* otherEntry = new Entry();
    otherEntry->setUuid(QUuid::createUuid());
    otherEntry->setTitle("Some Website 2");
    otherEntry->setPassword("secretsecretsecret 2");
    otherGroup->addEntry(otherEntry);

    oldGroup = targetDatabase->setRootGroup(otherRootGroup);
    delete oldGroup;

    sourceDatabase->saveAs(sourceDatabaseFilename);
    targetDatabase->saveAs(targetDatabaseFilename);

    setInput({"b", "a"});
    execCmd(mergeCmd,
            {"merge",
             "-k",
             targetKeyfilePath,
             "--key-file-from",
             sourceKeyfilePath,
             targetDatabaseFilename,
             sourceDatabaseFilename});

    QList<QByteArray> lines = m_stdout->readAll().split('\n');
    QVERIFY(lines.contains(
        QString("Successfully merged %1 into %2.").arg(sourceDatabaseFilename, targetDatabaseFilename).toUtf8()));
}

void TestCli::testMove()
{
    Move moveCmd;
    QVERIFY(!moveCmd.name.isEmpty());
    QVERIFY(moveCmd.getDescriptionLine().contains(moveCmd.name));

    setInput("a");
    execCmd(moveCmd, {"mv", m_dbFile->fileName(), "invalid_entry_path", "invalid_group_path"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readLine(), QByteArray("Could not find entry with path invalid_entry_path.\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray());

    setInput("a");
    execCmd(moveCmd, {"mv", m_dbFile->fileName(), "Sample Entry", "invalid_group_path"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readLine(), QByteArray("Could not find group with path invalid_group_path.\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray());

    setInput("a");
    execCmd(moveCmd, {"mv", m_dbFile->fileName(), "Sample Entry", "General/"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readLine(), QByteArray());
    QCOMPARE(m_stdout->readLine(), QByteArray("Successfully moved entry Sample Entry to group General/.\n"));

    auto db = readDatabase();
    auto* entry = db->rootGroup()->findEntryByPath("General/Sample Entry");
    QVERIFY(entry);

    // Test that not modified if the same group is destination.
    setInput("a");
    execCmd(moveCmd, {"mv", m_dbFile->fileName(), "General/Sample Entry", "General/"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readLine(), QByteArray("Entry is already in group General/.\n"));
    QCOMPARE(m_stdout->readLine(), QByteArray());

    // sanity check
    db = readDatabase();
    entry = db->rootGroup()->findEntryByPath("General/Sample Entry");
    QVERIFY(entry);
}

static Group* addMoveDirGroup(Group* parent, const QString& name)
{
    auto* group = new Group();
    group->setUuid(QUuid::createUuid());
    group->setName(name);
    group->setParent(parent);
    return group;
}

static QList<QUuid> childUuids(const Group* group)
{
    QList<QUuid> uuids;
    for (const Group* child : group->children()) {
        uuids.append(child->uuid());
    }
    return uuids;
}

// Build ordinary-path fixtures without exercising mkdir for every mvdir case.
bool TestCli::prepareMoveDirGroups(const QStringList& paths)
{
    auto db = readDatabase();
    if (!db) {
        return false;
    }
    for (const QString& path : paths) {
        Group* parent = db->rootGroup();
        for (const QString& name : path.split('/', Qt::SkipEmptyParts)) {
            Group* child = nullptr;
            for (Group* candidate : parent->children()) {
                if (candidate->name() == name) {
                    child = candidate;
                    break;
                }
            }
            if (!child) {
                child = addMoveDirGroup(parent, name);
            }
            parent = child;
        }
    }
    return db->save(Database::Atomic);
}

void TestCli::testMoveDir()
{
    MoveDir moveDirCmd;
    QVERIFY(!moveDirCmd.name.isEmpty());
    QVERIFY(moveDirCmd.getDescriptionLine().contains(moveDirCmd.name));

    QVERIFY(prepareMoveDirGroups({"/Source", "/Source/Child", "/Archive"}));

    Add addCmd;
    setInput("a");
    QCOMPARE(execCmd(addCmd, {"add", "-u", "preserved", m_dbFile->fileName(), "/Source/Child/Entry"}), EXIT_SUCCESS);
    m_stderr->readLine(); // Skip password prompt
    m_stderr->readAll();
    m_stdout->readAll();

    auto originalDb = readDatabase();
    QVERIFY(originalDb);
    Group* originalSource = originalDb->rootGroup()->findGroupByPath("/Source");
    Group* originalChild = originalDb->rootGroup()->findGroupByPath("/Source/Child");
    Entry* originalEntry = originalDb->rootGroup()->findEntryByPath("/Source/Child/Entry");
    Group* archive = originalDb->rootGroup()->findGroupByPath("/Archive");
    QVERIFY(originalSource);
    QVERIFY(originalChild);
    QVERIFY(originalEntry);
    QVERIFY(archive);

    const QUuid sourceUuid = originalSource->uuid();
    const QUuid childUuid = originalChild->uuid();
    const QUuid entryUuid = originalEntry->uuid();
    const QUuid customIconUuid = QUuid::createUuid();
    const QByteArray customIconData("mvdir custom icon bytes");
    originalDb->metadata()->addCustomIcon(customIconUuid, customIconData, "mvdir test icon");
    originalSource->setNotes("group notes survive mvdir");
    originalSource->setTags("group-tag");
    originalSource->setAutoTypeEnabled(Group::Disable);
    originalSource->setSearchingEnabled(Group::Disable);
    originalSource->setDefaultAutoTypeSequence("{USERNAME}{TAB}{PASSWORD}{ENTER}");
    originalSource->setIcon(customIconUuid);
    originalSource->customData()->set("mvdir-test", "group custom data");
    originalEntry->setPassword("current-secret");
    originalEntry->setNotes("entry notes survive mvdir");
    originalEntry->attributes()->set("Custom field", "custom value");
    originalEntry->attachments()->set("mvdir.txt", QByteArray("attachment contents"));
    originalEntry->setIcon(customIconUuid);
    auto* historyEntry = originalEntry->clone(Entry::CloneNoFlags);
    historyEntry->setPassword("previous-secret");
    originalEntry->addHistoryItem(historyEntry);

    auto* referenceEntry = new Entry();
    referenceEntry->setUuid(QUuid::createUuid());
    referenceEntry->setTitle("Reference to moved entry");
    referenceEntry->setPassword(QString("{REF:P@I:%1}").arg(originalEntry->uuidToHex()));
    referenceEntry->setGroup(archive);
    auto* sameNameEntry = new Entry();
    sameNameEntry->setUuid(QUuid::createUuid());
    sameNameEntry->setTitle("Renamed");
    sameNameEntry->setGroup(archive);
    const QUuid sameNameEntryUuid = sameNameEntry->uuid();
    QString saveError;
    QVERIFY2(originalDb->save(Database::Atomic, {}, &saveError), qPrintable(saveError));

    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "/Source", "/Archive/Renamed"}), EXIT_SUCCESS);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully moved group /Source to /Archive/Renamed.\n"));

    auto db = readDatabase();
    QVERIFY(db->rootGroup()->findGroupByPath("/Archive/Renamed"));
    QVERIFY(!db->rootGroup()->findGroupByPath("/Source"));
    QVERIFY(db->rootGroup()->findGroupByPath("/Archive/Renamed/Child"));
    auto* entry = db->rootGroup()->findEntryByPath("/Archive/Renamed/Child/Entry");
    QVERIFY(entry);
    auto* movedSource = db->rootGroup()->findGroupByPath("/Archive/Renamed");
    auto* movedChild = db->rootGroup()->findGroupByPath("/Archive/Renamed/Child");
    QCOMPARE(movedSource->uuid(), sourceUuid);
    QCOMPARE(movedChild->uuid(), childUuid);
    QCOMPARE(entry->uuid(), entryUuid);
    QCOMPARE(entry->username(), QString("preserved"));
    QCOMPARE(entry->password(), QString("current-secret"));
    QCOMPARE(entry->notes(), QString("entry notes survive mvdir"));
    QCOMPARE(entry->attributes()->value("Custom field"), QString("custom value"));
    QCOMPARE(entry->attachments()->value("mvdir.txt"), QByteArray("attachment contents"));
    QCOMPARE(entry->historyItems().size(), 1);
    QCOMPARE(entry->historyItems().first()->password(), QString("previous-secret"));
    QCOMPARE(movedSource->notes(), QString("group notes survive mvdir"));
    QCOMPARE(movedSource->tags(), QString("group-tag"));
    QCOMPARE(movedSource->autoTypeEnabled(), Group::Disable);
    QCOMPARE(movedSource->searchingEnabled(), Group::Disable);
    QCOMPARE(movedSource->defaultAutoTypeSequence(), QString("{USERNAME}{TAB}{PASSWORD}{ENTER}"));
    QCOMPARE(movedSource->iconUuid(), customIconUuid);
    QCOMPARE(entry->iconUuid(), customIconUuid);
    QVERIFY(db->metadata()->hasCustomIcon(customIconUuid));
    QCOMPARE(db->metadata()->customIcon(customIconUuid).data, customIconData);
    QCOMPARE(db->metadata()->customIcon(customIconUuid).name, QString("mvdir test icon"));
    QCOMPARE(movedSource->customData()->value("mvdir-test"), QString("group custom data"));
    QCOMPARE(db->rootGroup()->findEntryByPath("/Archive/Renamed")->uuid(), sameNameEntryUuid);
    auto* movedReference = db->rootGroup()->findEntryByPath("/Archive/Reference to moved entry");
    QVERIFY(movedReference);
    QVERIFY(movedReference->hasReferencesTo(entryUuid));
    QCOMPARE(movedReference->resolveMultiplePlaceholders(movedReference->password()), QString("current-secret"));

    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", "-q", m_dbFile->fileName(), "/Archive/Renamed", "/Quiet"}), EXIT_SUCCESS);
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray());
    db = readDatabase();
    QVERIFY(db);
    QCOMPARE(db->rootGroup()->findGroupByPath("/Quiet")->uuid(), sourceUuid);
}

void TestCli::testMoveDirOperations_data()
{
    QTest::addColumn<QString>("sourcePath");
    QTest::addColumn<QString>("sourceStoragePath");
    QTest::addColumn<QString>("destinationPath");
    QTest::addColumn<QString>("destinationStoragePath");
    QTest::addColumn<bool>("populated");
    QTest::addColumn<bool>("sameParent");
    QTest::addColumn<QString>("siblingPosition");

    QTest::newRow("rename-first-sibling-preserves-order")
        << QString("/Parent/First") << QString("/Parent/First") << QString("/Parent/RenamedFirst")
        << QString("/Parent/RenamedFirst") << false << true << QString("first");
    QTest::newRow("rename-middle-sibling-preserves-order")
        << QString("/Parent/Middle") << QString("/Parent/Middle") << QString("/Parent/RenamedMiddle")
        << QString("/Parent/RenamedMiddle") << false << true << QString("middle");
    QTest::newRow("rename-last-sibling-preserves-order")
        << QString("/Parent/Last") << QString("/Parent/Last") << QString("/Parent/RenamedLast")
        << QString("/Parent/RenamedLast") << false << true << QString("last");
    QTest::newRow("move-populated-to-root") << QString("/Parent/Source") << QString("/Parent/Source")
                                            << QString("/Source") << QString("/Source") << true << false << QString();
    QTest::newRow("move-only-to-nested-parent")
        << QString("/Source") << QString("/Source") << QString("/Archive/Source") << QString("/Archive/Source") << true
        << false << QString();
    QTest::newRow("move-and-rename") << QString("/Source") << QString("/Source") << QString("/Archive/Renamed")
                                     << QString("/Archive/Renamed") << true << false << QString();
    QTest::newRow("move-empty-group") << QString("/Parent/Empty") << QString("/Parent/Empty")
                                      << QString("/Archive/Empty") << QString("/Archive/Empty") << false << false
                                      << QString();
    QTest::newRow("case-only-rename") << QString("/Archive/Case") << QString("/Archive/Case")
                                      << QString("/Archive/case") << QString("/Archive/case") << false << true
                                      << QString();
    QTest::newRow("normalize-repeated-separators")
        << QString("/Parent//Source//") << QString("/Parent/Source") << QString("//Archive///Moved//")
        << QString("/Archive/Moved") << true << false << QString();
    QTest::newRow("paths-without-leading-separator")
        << QString("Source") << QString("/Source") << QString("Archive/Bare") << QString("/Archive/Bare") << false
        << false << QString();
    QTest::newRow("dot-components-are-literal-names")
        << QString("/Parent/../Source") << QString("/Parent/../Source") << QString("/Archive/./Moved")
        << QString("/Archive/./Moved") << false << false << QString();
    QTest::newRow("unicode-and-space-names")
        << QString("/Parent With Space/Grüße") << QString("/Parent With Space/Grüße")
        << QString("/Parent With Space/Zażółć") << QString("/Parent With Space/Zażółć") << false << true << QString();
}

void TestCli::testMoveDirOperations()
{
    QFETCH(QString, sourcePath);
    QFETCH(QString, sourceStoragePath);
    QFETCH(QString, destinationPath);
    QFETCH(QString, destinationStoragePath);
    QFETCH(bool, populated);
    QFETCH(bool, sameParent);
    QFETCH(QString, siblingPosition);

    const QString sourceParentPath = sourceStoragePath.left(sourceStoragePath.lastIndexOf('/'));
    const QString destinationParentPath = destinationStoragePath.left(destinationStoragePath.lastIndexOf('/'));
    QStringList paths;
    if (!sameParent) {
        paths.append(destinationParentPath + "/ExistingDestinationChild");
    }
    if (!siblingPosition.isEmpty() && siblingPosition != "first") {
        paths.append(sourceParentPath + "/Before");
    }
    paths.append(sourceStoragePath);
    if (!siblingPosition.isEmpty() && siblingPosition != "last") {
        paths.append(sourceParentPath + "/After");
    }
    paths.append("/Unrelated");
    if (populated) {
        paths.append(sourceStoragePath + "/Child");
    }
    QVERIFY(prepareMoveDirGroups(paths));

    auto db = readDatabase();
    QVERIFY(db);
    Group* root = db->rootGroup();
    Group* source = root->findGroupByPath(sourceStoragePath);
    QVERIFY(source);
    QHash<QUuid, QString> groupPathSuffixes;
    for (Group* group : source->groupsRecursive(true)) {
        groupPathSuffixes.insert(group->uuid(), group->fullPath().mid(source->fullPath().size()));
    }
    Group* unrelated = root->findGroupByPath("/Unrelated");
    QVERIFY(unrelated);
    const QUuid unrelatedUuid = unrelated->uuid();
    Group* originalParent = source->parentGroup();
    const QUuid originalParentUuid = originalParent->uuid();
    const QUuid originalPreviousParentUuid = source->previousParentGroupUuid();
    const QDateTime originalLocationChanged = source->timeInfo().locationChanged();
    Group* originalDestinationParent = root->findGroupByPath(destinationParentPath);
    QVERIFY(originalDestinationParent);
    const QList<QUuid> originalDestinationChildOrder = childUuids(originalDestinationParent);
    const bool movesToDifferentParent = originalDestinationParent != originalParent;
    const QUuid sourceUuid = source->uuid();

    MockClock::setup(new MockClock(2026, 11, 8, 12, 34, 56));
    const auto restoreMockClock = qScopeGuard([] { MockClock::teardown(); });

    MoveDir moveDirCmd;
    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), sourcePath, destinationPath}), EXIT_SUCCESS);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray(qPrintable(QString("Successfully moved group %1 to %2.\n").arg(sourcePath, destinationPath))));

    db = readDatabase();
    QVERIFY(db);
    root = db->rootGroup();
    const QString expectedFullPathPrefix = root->fullPath() + destinationStoragePath;
    Group* moved = root->findGroupByPath(destinationStoragePath);
    QVERIFY(moved);
    QCOMPARE(moved->uuid(), sourceUuid);
    Group* expectedParent = root->findGroupByPath(destinationParentPath);
    QCOMPARE(moved->parentGroup(), expectedParent);
    QList<QUuid> expectedDestinationChildOrder = originalDestinationChildOrder;
    if (movesToDifferentParent) {
        expectedDestinationChildOrder.append(sourceUuid);
    }
    QCOMPARE(childUuids(expectedParent), expectedDestinationChildOrder);
    if (sameParent) {
        QCOMPARE(moved->previousParentGroupUuid(), originalPreviousParentUuid);
        QCOMPARE(moved->timeInfo().locationChanged(), originalLocationChanged);
    } else {
        QCOMPARE(moved->previousParentGroupUuid(), originalParentUuid);
        QCOMPARE(moved->timeInfo().locationChanged(), MockClock::datetimeUtc(2026, 11, 8, 12, 34, 56));
    }
    if (sourceStoragePath != destinationStoragePath) {
        QVERIFY(!root->findGroupByPath(sourceStoragePath));
    }
    for (auto iter = groupPathSuffixes.constBegin(); iter != groupPathSuffixes.constEnd(); ++iter) {
        Group* group = root->findGroupByUuid(iter.key());
        QVERIFY(group);
        QCOMPARE(group->fullPath(), expectedFullPathPrefix + iter.value());
    }
    QCOMPARE(root->findGroupByPath("/Unrelated")->uuid(), unrelatedUuid);
}

void TestCli::testMoveDirRejected_data()
{
    QTest::addColumn<QString>("sourcePath");
    QTest::addColumn<QString>("destinationPath");
    QTest::addColumn<QString>("expectedError");

    QTest::newRow("missing-source") << QString("/Missing") << QString("/Archive/New")
                                    << QString("Could not find group with path /Missing.\n");
    QTest::newRow("missing-destination-parent")
        << QString("/Source") << QString("/Missing/New") << QString("Destination parent group Missing not found.\n");
    QTest::newRow("existing-destination") << QString("/Source") << QString("/Archive/Existing")
                                          << QString("Destination group /Archive/Existing already exists.\n");
    QTest::newRow("existing-destination-with-path-alias")
        << QString("/Source") << QString("/Archive//Existing///")
        << QString("Destination group /Archive//Existing/// already exists.\n");
    QTest::newRow("existing-descendant-destination") << QString("/Source") << QString("/Source/Child")
                                                     << QString("Destination group /Source/Child already exists.\n");
    QTest::newRow("exact-self-target") << QString("/Source") << QString("/Source")
                                       << QString("Destination group /Source already exists.\n");
    QTest::newRow("cycle-immediate-child") << QString("/Source") << QString("/Source/Child/Moved")
                                           << QString("Cannot move a group into itself or one of its descendants.\n");
    QTest::newRow("cycle-deep-descendant") << QString("/Source") << QString("/Source/Child/Deep/Moved")
                                           << QString("Cannot move a group into itself or one of its descendants.\n");
    QTest::newRow("root-protected") << QString("/") << QString("/RenamedRoot")
                                    << QString("Cannot move root group from database.\n");
    QTest::newRow("root-protected-repeated-slashes")
        << QString("////") << QString("/RenamedRoot") << QString("Cannot move root group from database.\n");
    QTest::newRow("empty-destination") << QString("/Source") << QString("")
                                       << QString("Destination path  is invalid.\n");
    QTest::newRow("slash-only-destination")
        << QString("/Source") << QString("///") << QString("Destination path /// is invalid.\n");
    QTest::newRow("similar-prefix-is-not-descendant") << QString("/A") << QString("/AB/Moved") << QString();
}

void TestCli::testMoveDirRejected()
{
    QFETCH(QString, sourcePath);
    QFETCH(QString, destinationPath);
    QFETCH(QString, expectedError);

    QVERIFY(prepareMoveDirGroups({"/Source/Child/Deep", "/Archive/Existing", "/Unrelated", "/AB", "/A"}));

    QFile databaseFile(m_dbFile->fileName());
    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    const QByteArray originalFile = databaseFile.readAll();
    databaseFile.close();

    MoveDir moveDirCmd;
    setInput("a");
    const int expectedResult = expectedError.isEmpty() ? EXIT_SUCCESS : EXIT_FAILURE;
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), sourcePath, destinationPath}), expectedResult);
    m_stderr->readLine(); // Skip password prompt
    const QByteArray commandError = m_stderr->readAll();
    const QByteArray commandOutput = m_stdout->readAll();
    if (expectedError.isEmpty()) {
        QCOMPARE(commandError, QByteArray());
        QVERIFY(commandOutput.startsWith("Successfully moved group /A to /AB/Moved."));
    } else {
        QCOMPARE(commandError, QByteArray(qPrintable(expectedError)));
        QCOMPARE(commandOutput, QByteArray());
    }

    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    const QByteArray currentFile = databaseFile.readAll();
    if (expectedError.isEmpty()) {
        QVERIFY(currentFile != originalFile);
    } else {
        QCOMPARE(currentFile, originalFile);
    }

    auto db = readDatabase();
    QVERIFY(db);
    if (!expectedError.isEmpty()) {
        QVERIFY(db->rootGroup()->findGroupByPath("/Source"));
        QVERIFY(!db->rootGroup()->findGroupByPath("/Archive/New"));
        QVERIFY(!db->rootGroup()->findGroupByPath("/Archive/Existing/New"));
    } else {
        QVERIFY(db->rootGroup()->findGroupByPath("/AB/Moved"));
        QVERIFY(!db->rootGroup()->findGroupByPath("/A"));
    }
}

void TestCli::testMoveDirAmbiguousPaths()
{
    QVERIFY(prepareMoveDirGroups({"/Parent"}));

    auto db = readDatabase();
    QVERIFY(db);
    Group* parent = db->rootGroup()->findGroupByPath("/Parent");
    QVERIFY(parent);
    Group* firstTwin = addMoveDirGroup(parent, "Twin");
    addMoveDirGroup(parent, "Twin");
    Group* slashNamedGroup = addMoveDirGroup(parent, "Slash/Name");
    Group* slashGroup = addMoveDirGroup(parent, "Slash");
    auto* nestedNameGroup = addMoveDirGroup(slashGroup, "Name");

    const QUuid slashNamedUuid = slashNamedGroup->uuid();
    const QUuid nestedNameUuid = nestedNameGroup->uuid();
    QCOMPARE(parent->findGroupByPath("Twin"), firstTwin);
    QCOMPARE(db->rootGroup()->findGroupByPath("/Parent/Slash/Name"), slashNamedGroup);
    QString saveError;
    QVERIFY2(db->save(Database::Atomic, {}, &saveError), qPrintable(saveError));

    QFile databaseFile(m_dbFile->fileName());
    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    const QByteArray originalFile = databaseFile.readAll();
    databaseFile.close();

    MoveDir moveDirCmd;
    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "/Parent/Twin", "/MovedTwin"}), EXIT_FAILURE);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Group path /Parent/Twin is ambiguous.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "/Parent/Slash/Name", "/MovedSlashName"}),
             EXIT_FAILURE);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Group path /Parent/Slash/Name is ambiguous.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "/Parent/Slash", "/Parent/Twin/Moved"}), EXIT_FAILURE);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Destination parent group Parent/Twin is ambiguous.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    QCOMPARE(databaseFile.readAll(), originalFile);

    db = readDatabase();
    QVERIFY(db);
    QCOMPARE(db->rootGroup()->findGroupByPath("/Parent/Slash/Name")->uuid(), slashNamedUuid);
    Group* nestedName = db->rootGroup()->findGroupByUuid(nestedNameUuid);
    QVERIFY(nestedName);
    QCOMPARE(nestedName->parentGroup()->name(), QString("Slash"));
}

void TestCli::testMoveDirStoredNameAliases_data()
{
    QTest::addColumn<QString>("destinationPath");
    QTest::addColumn<QString>("storedName");
    QTest::addColumn<QString>("sourcePath");
    QTest::addColumn<QStringList>("siblingNames");
    QTest::addColumn<QString>("expectedError");

    QTest::newRow("normal-and-trailing-separator-siblings")
        << QString("/Moved") << QString("A") << QString("/Parent/A") << QStringList{"A/"} << QString();
    QTest::newRow("trailing-separator-does-not-alias-normal-name")
        << QString("/Moved") << QString("A/") << QString("/Parent/A") << QStringList{}
        << QString("Could not find group with path /Parent/A.\n");
    QTest::newRow("trailing-separator-exact-spelling")
        << QString("/Moved") << QString("A/") << QString("/Parent/A//") << QStringList{} << QString();
    QTest::newRow("leading-separator-in-stored-name")
        << QString("/Moved") << QString("/Leading") << QString("/Parent//Leading") << QStringList{} << QString();
    QTest::newRow("doubled-separator-in-stored-name") << QString("/Moved") << QString("Double//Inner")
                                                      << QString("/Parent/Double//Inner") << QStringList{} << QString();
    QTest::newRow("trailing-separator-name-cannot-be-silently-shortened")
        << QString("/Other/A//") << QString("A/") << QString("/Parent/A//") << QStringList{}
        << QString("Destination path cannot preserve a group name containing '/'; choose a different final name.\n");
    QTest::newRow("slash-name-can-be-addressed-and-renamed")
        << QString("/Other/Renamed") << QString("Slash/Name") << QString("/Parent/Slash/Name") << QStringList{}
        << QString();
    QTest::newRow("trailing-separator-sibling-does-not-collide-with-destination")
        << QString("/Parent/A") << QString("Source") << QString("/Parent/Source") << QStringList{"A/"} << QString();
}

void TestCli::testMoveDirStoredNameAliases()
{
    QFETCH(QString, destinationPath);
    QFETCH(QString, storedName);
    QFETCH(QString, sourcePath);
    QFETCH(QStringList, siblingNames);
    QFETCH(QString, expectedError);

    QVERIFY(prepareMoveDirGroups({"/Parent", "/Other"}));

    auto db = readDatabase();
    QVERIFY(db);
    Group* parent = db->rootGroup()->findGroupByPath("/Parent");
    QVERIFY(parent);
    Group* source = addMoveDirGroup(parent, storedName);
    const QUuid sourceUuid = source->uuid();
    const QUuid parentUuid = parent->uuid();
    QHash<QString, QUuid> siblingUuids;
    for (const QString& name : siblingNames) {
        siblingUuids.insert(name, addMoveDirGroup(parent, name)->uuid());
    }
    QString saveError;
    QVERIFY2(db->save(Database::Atomic, {}, &saveError), qPrintable(saveError));

    QFile databaseFile(m_dbFile->fileName());
    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    const QByteArray originalFile = databaseFile.readAll();
    databaseFile.close();

    MoveDir moveDirCmd;
    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), sourcePath, destinationPath}),
             expectedError.isEmpty() ? EXIT_SUCCESS : EXIT_FAILURE);
    m_stderr->readLine(); // Skip password prompt
    if (expectedError.isEmpty()) {
        QCOMPARE(m_stderr->readAll(), QByteArray());
        QCOMPARE(
            m_stdout->readAll(),
            QByteArray(qPrintable(QString("Successfully moved group %1 to %2.\n").arg(sourcePath, destinationPath))));
    } else {
        QCOMPARE(m_stderr->readAll(), QByteArray(qPrintable(expectedError)));
        QCOMPARE(m_stdout->readAll(), QByteArray());
    }

    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    if (expectedError.isEmpty()) {
        QVERIFY(databaseFile.readAll() != originalFile);
    } else {
        QCOMPARE(databaseFile.readAll(), originalFile);
    }

    db = readDatabase();
    QVERIFY(db);
    Group* resolvedSource = db->rootGroup()->findGroupByUuid(sourceUuid);
    QVERIFY(resolvedSource);
    if (expectedError.isEmpty()) {
        QCOMPARE(resolvedSource->name(), destinationPath.section('/', -1));
        QCOMPARE(resolvedSource, db->rootGroup()->findGroupByPath(destinationPath));
    } else {
        QCOMPARE(resolvedSource->name(), storedName);
        QCOMPARE(resolvedSource->parentGroup()->uuid(), parentUuid);
    }
    for (auto iter = siblingUuids.constBegin(); iter != siblingUuids.constEnd(); ++iter) {
        Group* sibling = db->rootGroup()->findGroupByUuid(iter.value());
        QVERIFY(sibling);
        QCOMPARE(sibling->name(), iter.key());
        QCOMPARE(sibling->parentGroup()->uuid(), parentUuid);
    }
}

void TestCli::testMoveDirLiteralDoubleSeparatorAndFallback()
{
    QVERIFY(prepareMoveDirGroups({"/Parent"}));

    auto db = readDatabase();
    QVERIFY(db);
    Group* parent = db->rootGroup()->findGroupByPath("/Parent");
    QVERIFY(parent);
    auto* literal = addMoveDirGroup(parent, "A//B");
    auto* nestedA = addMoveDirGroup(parent, "A");
    auto* nestedB = addMoveDirGroup(nestedA, "B");
    const QUuid literalUuid = literal->uuid();
    const QUuid nestedAUuid = nestedA->uuid();
    const QUuid nestedBUuid = nestedB->uuid();
    QString saveError;
    QVERIFY2(db->save(Database::Atomic, {}, &saveError), qPrintable(saveError));

    MoveDir moveDirCmd;
    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "/Parent/A//B", "/MovedLiteral"}), EXIT_SUCCESS);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully moved group /Parent/A//B to /MovedLiteral.\n"));

    db = readDatabase();
    QVERIFY(db);
    Group* movedLiteral = db->rootGroup()->findGroupByUuid(literalUuid);
    QVERIFY(movedLiteral);
    QCOMPARE(movedLiteral->name(), QString("MovedLiteral"));
    QCOMPARE(movedLiteral->parentGroup(), db->rootGroup());
    QCOMPARE(db->rootGroup()->findGroupByUuid(nestedAUuid)->parentGroup(), db->rootGroup()->findGroupByPath("/Parent"));
    QCOMPARE(db->rootGroup()->findGroupByUuid(nestedBUuid)->parentGroup()->uuid(), nestedAUuid);

    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "/Parent/A///B", "/MovedNested"}), EXIT_SUCCESS);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully moved group /Parent/A///B to /MovedNested.\n"));

    auto reopenedDb = readDatabase();
    QVERIFY(reopenedDb);
    movedLiteral = reopenedDb->rootGroup()->findGroupByUuid(literalUuid);
    QVERIFY(movedLiteral);
    QCOMPARE(movedLiteral->name(), QString("MovedLiteral"));
    QCOMPARE(movedLiteral->parentGroup(), reopenedDb->rootGroup());
    Group* movedNested = reopenedDb->rootGroup()->findGroupByUuid(nestedBUuid);
    QVERIFY(movedNested);
    QCOMPARE(movedNested->name(), QString("MovedNested"));
    QCOMPARE(movedNested->parentGroup(), reopenedDb->rootGroup());
    QCOMPARE(reopenedDb->rootGroup()->findGroupByUuid(nestedAUuid)->parentGroup(),
             reopenedDb->rootGroup()->findGroupByPath("/Parent"));
}

void TestCli::testMoveDirDestinationChildCollisionWithStoredSeparator()
{
    QVERIFY(prepareMoveDirGroups({"/Source"}));

    auto db = readDatabase();
    QVERIFY(db);
    auto* source = db->rootGroup()->findGroupByPath("/Source");
    QVERIFY(source);
    const QUuid sourceUuid = source->uuid();

    auto* parent = addMoveDirGroup(db->rootGroup(), "A/");
    const QUuid parentUuid = parent->uuid();
    auto* existingChild = addMoveDirGroup(parent, "New");
    const QUuid childUuid = existingChild->uuid();

    QString saveError;
    QVERIFY2(db->save(Database::Atomic, {}, &saveError), qPrintable(saveError));
    QFile databaseFile(m_dbFile->fileName());
    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    const QByteArray originalFile = databaseFile.readAll();
    databaseFile.close();

    MoveDir moveDirCmd;
    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "/Source", "/A///New"}), EXIT_FAILURE);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Destination group /A///New already exists.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    QCOMPARE(databaseFile.readAll(), originalFile);

    auto reopenedDb = readDatabase();
    QVERIFY(reopenedDb);
    Group* unchangedSource = reopenedDb->rootGroup()->findGroupByUuid(sourceUuid);
    Group* unchangedParent = reopenedDb->rootGroup()->findGroupByUuid(parentUuid);
    Group* unchangedChild = reopenedDb->rootGroup()->findGroupByUuid(childUuid);
    QVERIFY(unchangedSource);
    QVERIFY(unchangedParent);
    QVERIFY(unchangedChild);
    QCOMPARE(unchangedSource->name(), QString("Source"));
    QCOMPARE(unchangedSource->parentGroup(), reopenedDb->rootGroup());
    QCOMPARE(unchangedParent->name(), QString("A/"));
    QCOMPARE(unchangedParent->parentGroup(), reopenedDb->rootGroup());
    QCOMPARE(unchangedChild->name(), QString("New"));
    QCOMPARE(unchangedChild->parentGroup(), unchangedParent);
}

void TestCli::testMoveDirRootResolutionWithSpecialNames()
{
    QVERIFY(prepareMoveDirGroups({"/Parent"}));

    auto db = readDatabase();
    QVERIFY(db);
    Group* parent = db->rootGroup()->findGroupByPath("/Parent");
    QVERIFY(parent);
    Group* source = addMoveDirGroup(parent, "Source");
    Group* emptyNamedGroup = addMoveDirGroup(parent, "");
    Group* slashNamedGroup = addMoveDirGroup(parent, "/");
    const QUuid sourceUuid = source->uuid();
    const QUuid emptyNamedUuid = emptyNamedGroup->uuid();
    const QUuid slashNamedUuid = slashNamedGroup->uuid();
    const QUuid rootUuid = db->rootGroup()->uuid();
    const QUuid parentUuid = parent->uuid();
    QString saveError;
    QVERIFY2(db->save(Database::Atomic, {}, &saveError), qPrintable(saveError));

    QFile databaseFile(m_dbFile->fileName());
    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    const QByteArray originalFile = databaseFile.readAll();
    databaseFile.close();

    MoveDir moveDirCmd;
    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "///", "/RootRenamed"}), EXIT_FAILURE);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Cannot move root group from database.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    QCOMPARE(databaseFile.readAll(), originalFile);
    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "/Parent/Source", "/Moved"}), EXIT_SUCCESS);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully moved group /Parent/Source to /Moved.\n"));

    db = readDatabase();
    QVERIFY(db);
    QCOMPARE(db->rootGroup()->uuid(), rootUuid);
    QCOMPARE(db->rootGroup()->findGroupByPath("/Moved")->uuid(), sourceUuid);
    Group* movedEmptyNamed = db->rootGroup()->findGroupByUuid(emptyNamedUuid);
    Group* movedSlashNamed = db->rootGroup()->findGroupByUuid(slashNamedUuid);
    QVERIFY(movedEmptyNamed);
    QVERIFY(movedSlashNamed);
    QCOMPARE(movedEmptyNamed->name(), QString());
    QCOMPARE(movedSlashNamed->name(), QString("/"));
    QCOMPARE(movedEmptyNamed->parentGroup()->uuid(), parentUuid);
    QCOMPARE(movedSlashNamed->parentGroup()->uuid(), parentUuid);
}

void TestCli::testMoveDirRecycleBin()
{
    QVERIFY(prepareMoveDirGroups({"/Archive"}));

    auto db = readDatabase();
    QVERIFY(db);
    auto* recycledGroup = addMoveDirGroup(db->rootGroup(), "Previously recycled group");
    const QUuid recycledGroupUuid = recycledGroup->uuid();
    db->recycleGroup(recycledGroup);

    auto* templatesGroup = addMoveDirGroup(db->rootGroup(), "Templates");
    const QUuid templatesGroupUuid = templatesGroup->uuid();
    db->metadata()->setEntryTemplatesGroup(templatesGroup);

    Group* recycleBin = db->metadata()->recycleBin();
    QVERIFY(recycleBin);
    const QUuid recycleBinUuid = recycleBin->uuid();
    const QString recycleBinPath = "/" + recycleBin->name();
    QString saveError;
    QVERIFY2(db->save(Database::Atomic, {}, &saveError), qPrintable(saveError));

    MoveDir moveDirCmd;
    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), recycleBinPath, "/Archive/Trash"}), EXIT_SUCCESS);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    m_stdout->readAll();

    db = readDatabase();
    QVERIFY(db);
    recycleBin = db->metadata()->recycleBin();
    QVERIFY(recycleBin);
    QCOMPARE(recycleBin->uuid(), recycleBinUuid);
    QCOMPARE(recycleBin, db->rootGroup()->findGroupByUuid(recycleBinUuid));
    QCOMPARE(db->rootGroup()->findGroupByPath("/Archive/Trash"), recycleBin);
    Group* movedRecycledGroup = db->rootGroup()->findGroupByUuid(recycledGroupUuid);
    QVERIFY(movedRecycledGroup);
    QCOMPARE(movedRecycledGroup->parentGroup(), recycleBin);
    QVERIFY(movedRecycledGroup->isRecycled());

    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "/Templates", "/Archive/TemplateShelf"}),
             EXIT_SUCCESS);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    m_stdout->readAll();

    db = readDatabase();
    QVERIFY(db);
    const Group* movedTemplatesGroup = db->metadata()->entryTemplatesGroup();
    QVERIFY(movedTemplatesGroup);
    QCOMPARE(movedTemplatesGroup->uuid(), templatesGroupUuid);
    QCOMPARE(movedTemplatesGroup, db->rootGroup()->findGroupByPath("/Archive/TemplateShelf"));
}

void TestCli::testMoveDirSaveFailureRestoresInteractiveState()
{
    QVERIFY(prepareMoveDirGroups({"/Source", "/Source/Child", "/Archive", "/Other"}));

    Add addCmd;
    setInput("a");
    QCOMPARE(execCmd(addCmd, {"add", "-u", "rollback-content", m_dbFile->fileName(), "/Source/Child/Entry"}),
             EXIT_SUCCESS);
    m_stderr->readLine(); // Skip password prompt
    m_stderr->readAll();
    m_stdout->readAll();

    MoveDir moveDirCmd;
    setInput("a");
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", m_dbFile->fileName(), "/Source", "/Archive/Source"}), EXIT_SUCCESS);
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    m_stdout->readAll();

    auto db = readDatabase();
    QVERIFY(db);
    const QString databasePath = m_dbFile->fileName();

    QFile databaseFile(databasePath);
    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    const QByteArray originalFile = databaseFile.readAll();
    databaseFile.close();

    Entry* originalEntry = db->rootGroup()->findEntryByPath("/Archive/Source/Child/Entry");
    QVERIFY(originalEntry);
    originalEntry->setPassword("rollback-secret");
    originalEntry->setNotes("rollback notes survive retry");
    db->rootGroup()->findGroupByPath("/Archive/Source")->setNotes("unsaved group changes survive rollback");
    db->metadata()->customData()->set("mvdir-rollback-test", "preserve me");

    Group* root = db->rootGroup();
    Group* source = root->findGroupByPath("/Archive/Source");
    QVERIFY(source);
    const QUuid originalEntryUuid = originalEntry->uuid();
    Group* originalParent = source->parentGroup();
    const int originalIndex = originalParent->children().indexOf(source);
    const QUuid originalUuid = source->uuid();
    const TimeInfo originalTimeInfo = source->timeInfo();
    const QUuid originalPreviousParentUuid = source->previousParentGroupUuid();
    CustomData originalMetadataCustomData;
    originalMetadataCustomData.copyDataFrom(db->metadata()->customData());
    const QList<QUuid> originalOrder = childUuids(originalParent);
    QVERIFY(db->isModified());

    // A missing parent provides a deterministic save failure without changing the vault file.
    db->setFilePath(databasePath + "/missing/failed.kdbx");
    moveDirCmd.currentDatabase = db;
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", "/Archive/Source", "/Other/Renamed"}), EXIT_FAILURE);
    const QByteArray errorOutput = m_stderr->readAll();
    QVERIFY(errorOutput.contains("Writing the database failed"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    db->setFilePath(databasePath);
    root = db->rootGroup();
    source = root->findGroupByPath("/Archive/Source");
    QVERIFY(source);
    QCOMPARE(source->uuid(), originalUuid);
    QCOMPARE(source->parentGroup(), originalParent);
    QCOMPARE(originalParent->children().indexOf(source), originalIndex);
    QCOMPARE(source->timeInfo(), originalTimeInfo);
    QCOMPARE(source->previousParentGroupUuid(), originalPreviousParentUuid);
    QCOMPARE(root->findGroupByPath("/Other/Renamed"), nullptr);
    QCOMPARE(source->notes(), QString("unsaved group changes survive rollback"));
    QVERIFY(*db->metadata()->customData() == originalMetadataCustomData);
    Entry* rolledBackEntry = root->findEntryByPath("/Archive/Source/Child/Entry");
    QVERIFY(rolledBackEntry);
    QCOMPARE(rolledBackEntry->uuid(), originalEntryUuid);
    QCOMPARE(rolledBackEntry->username(), QString("rollback-content"));
    QCOMPARE(rolledBackEntry->password(), QString("rollback-secret"));
    QCOMPARE(rolledBackEntry->notes(), QString("rollback notes survive retry"));
    QCOMPARE(childUuids(originalParent), originalOrder);
    QVERIFY(db->isModified());

    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    QCOMPARE(databaseFile.readAll(), originalFile);
    databaseFile.close();

    QCOMPARE(execCmd(moveDirCmd, {"mvdir", "/Archive/Source", "/Other/Renamed"}), EXIT_SUCCESS);
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully moved group /Archive/Source to /Other/Renamed.\n"));
    QVERIFY(!db->isModified());
    auto reopenedDb = readDatabase();
    QVERIFY(reopenedDb);
    for (const auto& savedDb : {db, reopenedDb}) {
        Group* savedGroup = savedDb->rootGroup()->findGroupByPath("/Other/Renamed");
        QVERIFY(savedGroup);
        QCOMPARE(savedGroup->uuid(), originalUuid);
        QCOMPARE(savedGroup->notes(), QString("unsaved group changes survive rollback"));
        Entry* savedEntry = savedDb->rootGroup()->findEntryByPath("/Other/Renamed/Child/Entry");
        QVERIFY(savedEntry);
        QCOMPARE(savedEntry->uuid(), originalEntryUuid);
        QCOMPARE(savedEntry->username(), QString("rollback-content"));
        QCOMPARE(savedEntry->password(), QString("rollback-secret"));
        QCOMPARE(savedEntry->notes(), QString("rollback notes survive retry"));
        QCOMPARE(savedDb->metadata()->customData()->value("mvdir-rollback-test"), QString("preserve me"));
    }
}

void TestCli::testMoveDirRenameSaveFailureRestoresState()
{
    QVERIFY(prepareMoveDirGroups({"/Parent", "/Parent/Before", "/Parent/Source", "/Parent/After"}));

    auto db = readDatabase();
    QVERIFY(db);
    Group* source = db->rootGroup()->findGroupByPath("/Parent/Source");
    QVERIFY(source);
    const QUuid sourceUuid = source->uuid();
    Group* originalParent = source->parentGroup();
    const QUuid parentUuid = originalParent->uuid();
    const QUuid previousParentUuid = source->previousParentGroupUuid();
    const TimeInfo originalTimeInfo = source->timeInfo();
    const QList<QUuid> originalOrder = childUuids(originalParent);
    db->metadata()->customData()->set("rename-rollback-test", "preserve metadata");
    QVERIFY(db->save(Database::Atomic));
    CustomData originalMetadata;
    originalMetadata.copyDataFrom(db->metadata()->customData());

    QFile databaseFile(m_dbFile->fileName());
    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    const QByteArray originalFile = databaseFile.readAll();
    databaseFile.close();
    QVERIFY(!db->isModified());

    const QString databasePath = m_dbFile->fileName();
    db->setFilePath(databasePath + "/missing/failed.kdbx");
    MoveDir moveDirCmd;
    moveDirCmd.currentDatabase = db;
    QCOMPARE(execCmd(moveDirCmd, {"mvdir", "/Parent/Source", "/Parent/Renamed"}), EXIT_FAILURE);
    QVERIFY(m_stderr->readAll().contains("Writing the database failed"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    db->setFilePath(databasePath);
    source = db->rootGroup()->findGroupByUuid(sourceUuid);
    QVERIFY(source);
    QCOMPARE(source->name(), QString("Source"));
    QCOMPARE(source->parentGroup()->uuid(), parentUuid);
    QCOMPARE(source->uuid(), sourceUuid);
    QCOMPARE(source->previousParentGroupUuid(), previousParentUuid);
    QCOMPARE(source->timeInfo(), originalTimeInfo);
    QCOMPARE(*db->metadata()->customData(), originalMetadata);
    QVERIFY(!db->isModified());
    QCOMPARE(childUuids(originalParent), originalOrder);
    QCOMPARE(db->rootGroup()->findGroupByPath("/Parent/Renamed"), nullptr);
    QVERIFY(databaseFile.open(QIODevice::ReadOnly));
    QCOMPARE(databaseFile.readAll(), originalFile);
}

void TestCli::testRemove()
{
    Remove removeCmd;
    QVERIFY(!removeCmd.name.isEmpty());
    QVERIFY(removeCmd.getDescriptionLine().contains(removeCmd.name));

    // load test database and save a copy with disabled recycle bin
    auto db = readDatabase();
    QVERIFY(db);
    TemporaryFile fileCopy;
    fileCopy.open();
    fileCopy.close();

    db->metadata()->setRecycleBinEnabled(false);
    db->saveAs(fileCopy.fileName());

    // delete entry and verify
    setInput("a");
    execCmd(removeCmd, {"rm", m_dbFile->fileName(), "/Sample Entry"});
    m_stderr->readLine(); // skip password prompt
    // QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully recycled entry Sample Entry.\n"));

    auto readBackDb = readDatabase();
    QVERIFY(readBackDb);
    QVERIFY(!readBackDb->rootGroup()->findEntryByPath("/Sample Entry"));
    QVERIFY(readBackDb->rootGroup()->findEntryByPath(QString("/%1/Sample Entry").arg(Group::tr("Recycle Bin"))));

    // try again, this time without recycle bin
    setInput("a");
    execCmd(removeCmd, {"rm", fileCopy.fileName(), "/Sample Entry"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully deleted entry Sample Entry.\n"));

    readBackDb = readDatabase(fileCopy.fileName(), "a");
    QVERIFY(readBackDb);
    QVERIFY(!readBackDb->rootGroup()->findEntryByPath("/Sample Entry"));
    QVERIFY(!readBackDb->rootGroup()->findEntryByPath(QString("/%1/Sample Entry").arg(Group::tr("Recycle Bin"))));

    // finally, try deleting a non-existent entry
    setInput("a");
    execCmd(removeCmd, {"rm", fileCopy.fileName(), "/Sample Entry"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Entry /Sample Entry not found.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // try deleting a directory, should fail
    setInput("a");
    execCmd(removeCmd, {"rm", fileCopy.fileName(), "/General"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Entry /General not found.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());
}

void TestCli::testRemoveGroup()
{
    RemoveGroup removeGroupCmd;
    QVERIFY(!removeGroupCmd.name.isEmpty());
    QVERIFY(removeGroupCmd.getDescriptionLine().contains(removeGroupCmd.name));

    // try deleting a directory, should recycle it first.
    setInput("a");
    execCmd(removeGroupCmd, {"rmdir", m_dbFile->fileName(), "/General"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully recycled group /General.\n"));

    auto db = readDatabase();
    auto* group = db->rootGroup()->findGroupByPath("General");
    QVERIFY(!group);

    // try deleting a directory again, should delete it permanently.
    setInput("a");
    execCmd(removeGroupCmd, {"rmdir", m_dbFile->fileName(), "Recycle Bin/General"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("Successfully deleted group Recycle Bin/General.\n"));

    db = readDatabase();
    group = db->rootGroup()->findGroupByPath("Recycle Bin/General");
    QVERIFY(!group);

    // try deleting an invalid group, should fail.
    setInput("a");
    execCmd(removeGroupCmd, {"rmdir", m_dbFile->fileName(), "invalid"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Group invalid not found.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Should fail to remove the root group.
    setInput("a");
    execCmd(removeGroupCmd, {"rmdir", m_dbFile->fileName(), "/"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("Cannot remove root group from database.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());
}

void TestCli::testRemoveQuiet()
{
    Remove removeCmd;
    QVERIFY(!removeCmd.name.isEmpty());
    QVERIFY(removeCmd.getDescriptionLine().contains(removeCmd.name));

    // delete entry and verify
    setInput("a");
    execCmd(removeCmd, {"rm", "-q", m_dbFile->fileName(), "/Sample Entry"});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray());

    auto db = readDatabase();
    QVERIFY(db);

    QVERIFY(!db->rootGroup()->findEntryByPath("/Sample Entry"));
    QVERIFY(db->rootGroup()->findEntryByPath(QString("/%1/Sample Entry").arg(Group::tr("Recycle Bin"))));

    // remove the entry completely
    setInput("a");
    execCmd(removeCmd, {"rm", "-q", m_dbFile->fileName(), QString("/%1/Sample Entry").arg(Group::tr("Recycle Bin"))});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray());

    db = readDatabase();
    QVERIFY(!db->rootGroup()->findEntryByPath("/Sample Entry"));
    QVERIFY(!db->rootGroup()->findEntryByPath(QString("/%1/Sample Entry").arg(Group::tr("Recycle Bin"))));
}

void TestCli::testSearch()
{
    Search searchCmd;
    QVERIFY(!searchCmd.name.isEmpty());
    QVERIFY(searchCmd.getDescriptionLine().contains(searchCmd.name));

    setInput("a");
    execCmd(searchCmd, {"search", m_dbFile->fileName(), "Sample"});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("/Sample Entry\n"));

    // Quiet option
    setInput("a");
    execCmd(searchCmd, {"search", m_dbFile->fileName(), "-q", "Sample"});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(), QByteArray("/Sample Entry\n"));

    setInput("a");
    execCmd(searchCmd, {"search", m_dbFile->fileName(), "Does Not Exist"});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray("No results for that search term.\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // write a modified database
    auto db = readDatabase();
    QVERIFY(db);
    auto* group = db->rootGroup()->findGroupByPath("/General/");
    QVERIFY(group);
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setTitle("New Entry");
    group->addEntry(entry);

    TemporaryFile tmpFile;
    tmpFile.open();
    tmpFile.close();
    db->saveAs(tmpFile.fileName());

    setInput("a");
    execCmd(searchCmd, {"search", tmpFile.fileName(), "title:New"});
    QCOMPARE(m_stdout->readAll(), QByteArray("/General/New Entry\n"));

    setInput("a");
    execCmd(searchCmd, {"search", tmpFile.fileName(), "title:Entry"});
    QCOMPARE(m_stdout->readAll(),
             QByteArray("/Sample Entry\n/General/New Entry\n/Homebanking/Subgroup/Subgroup Entry\n"));

    setInput("a");
    execCmd(searchCmd, {"search", tmpFile.fileName(), "group:General"});
    QCOMPARE(m_stdout->readAll(), QByteArray("/General/New Entry\n"));

    setInput("a");
    execCmd(searchCmd, {"search", tmpFile.fileName(), "group:NewDatabase"});
    QCOMPARE(m_stdout->readAll(), QByteArray("/Sample Entry\n"));

    setInput("a");
    execCmd(searchCmd, {"search", tmpFile.fileName(), "group:/NewDatabase"});
    QCOMPARE(m_stdout->readAll(),
             QByteArray("/Sample Entry\n/General/New Entry\n/Homebanking/Subgroup/Subgroup Entry\n"));

    setInput("a");
    execCmd(searchCmd, {"search", tmpFile.fileName(), "url:bank"});
    QCOMPARE(m_stdout->readAll(), QByteArray("/Homebanking/Subgroup/Subgroup Entry\n"));

    setInput("a");
    execCmd(searchCmd, {"search", tmpFile.fileName(), "u:User Name"});
    QCOMPARE(m_stdout->readAll(), QByteArray("/Sample Entry\n/Homebanking/Subgroup/Subgroup Entry\n"));
}

void TestCli::testShow()
{
    Show showCmd;
    QVERIFY(!showCmd.name.isEmpty());
    QVERIFY(showCmd.getDescriptionLine().contains(showCmd.name));

    setInput("a");
    execCmd(showCmd, {"show", m_dbFile->fileName(), "/Sample Entry"});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Title: Sample Entry\n"
                        "UserName: User Name\n"
                        "Password: PROTECTED\n"
                        "URL: http://www.somesite.com/\n"
                        "Notes: Notes\n"
                        "Uuid: {9f4544c2-ab00-c74a-8a1a-6eaf26cf57e9}\n"
                        "Tags: \n"));

    setInput("a");
    execCmd(showCmd, {"show", "-s", m_dbFile->fileName(), "/Sample Entry"});
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Title: Sample Entry\n"
                        "UserName: User Name\n"
                        "Password: Password\n"
                        "URL: http://www.somesite.com/\n"
                        "Notes: Notes\n"
                        "Uuid: {9f4544c2-ab00-c74a-8a1a-6eaf26cf57e9}\n"
                        "Tags: \n"));

    setInput("a");
    execCmd(showCmd, {"show", m_dbFile->fileName(), "-q", "/Sample Entry"});
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Title: Sample Entry\n"
                        "UserName: User Name\n"
                        "Password: PROTECTED\n"
                        "URL: http://www.somesite.com/\n"
                        "Notes: Notes\n"
                        "Uuid: {9f4544c2-ab00-c74a-8a1a-6eaf26cf57e9}\n"
                        "Tags: \n"));

    setInput("a");
    execCmd(showCmd, {"show", m_dbFile->fileName(), "--show-attachments", "/Sample Entry"});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Title: Sample Entry\n"
                        "UserName: User Name\n"
                        "Password: PROTECTED\n"
                        "URL: http://www.somesite.com/\n"
                        "Notes: Notes\n"
                        "Uuid: {9f4544c2-ab00-c74a-8a1a-6eaf26cf57e9}\n"
                        "Tags: \n"
                        "\n"
                        "Attachments:\n"
                        "  Sample attachment.txt (15 B)\n"));

    setInput("a");
    execCmd(showCmd, {"show", m_dbFile->fileName(), "--show-attachments", "/Homebanking/Subgroup/Subgroup Entry"});
    m_stderr->readLine(); // Skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Title: Subgroup Entry\n"
                        "UserName: Bank User Name\n"
                        "Password: PROTECTED\n"
                        "URL: https://www.bank.com\n"
                        "Notes: Important note\n"
                        "Uuid: {20b183fd-6878-4506-a50b-06d30792aa10}\n"
                        "Tags: \n"
                        "\n"
                        "No attachments present.\n"));

    setInput("a");
    execCmd(showCmd, {"show", "-a", "Title", m_dbFile->fileName(), "/Sample Entry"});
    QCOMPARE(m_stdout->readAll(), QByteArray("Sample Entry\n"));

    setInput("a");
    execCmd(showCmd, {"show", "-a", "Password", m_dbFile->fileName(), "/Sample Entry"});
    QCOMPARE(m_stdout->readAll(), QByteArray("Password\n"));

    setInput("a");
    execCmd(showCmd, {"show", "-a", "Uuid", m_dbFile->fileName(), "/Sample Entry"});
    QCOMPARE(m_stdout->readAll(), QByteArray("{9f4544c2-ab00-c74a-8a1a-6eaf26cf57e9}\n"));

    setInput("a");
    execCmd(showCmd, {"show", "-a", "Title", "-a", "URL", m_dbFile->fileName(), "/Sample Entry"});
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Sample Entry\n"
                        "http://www.somesite.com/\n"));

    // Test case insensitivity
    setInput("a");
    execCmd(showCmd, {"show", "-a", "TITLE", "-a", "URL", m_dbFile->fileName(), "/Sample Entry"});
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Sample Entry\n"
                        "http://www.somesite.com/\n"));

    setInput("a");
    execCmd(showCmd, {"show", "-a", "DoesNotExist", m_dbFile->fileName(), "/Sample Entry"});
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QVERIFY(m_stderr->readAll().contains("ERROR: unknown attribute DoesNotExist.\n"));

    setInput("a");
    execCmd(showCmd, {"show", "-t", m_dbFile->fileName(), "/Sample Entry"});
    QVERIFY(isTotp(m_stdout->readAll()));

    setInput("a");
    execCmd(showCmd, {"show", "-a", "Title", m_dbFile->fileName(), "--totp", "/Sample Entry"});
    QCOMPARE(m_stdout->readLine(), QByteArray("Sample Entry\n"));
    QVERIFY(isTotp(m_stdout->readAll()));

    setInput("a");
    execCmd(showCmd, {"show", m_dbFile2->fileName(), "--totp", "/Sample Entry"});
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QVERIFY(m_stderr->readAll().contains("Entry with path /Sample Entry has no TOTP set up.\n"));

    // Show with ambiguous attributes
    setInput("a");
    execCmd(showCmd, {"show", m_dbFile->fileName(), "-a", "Testattribute1", "/Sample Entry"});
    QCOMPARE(m_stdout->readAll(), QByteArray());
    QVERIFY(m_stderr->readAll().contains("ERROR: attribute Testattribute1 is ambiguous"));

    setInput("a");
    execCmd(showCmd, {"show", "--all", m_dbFile->fileName(), "/Sample Entry"});
    QCOMPARE(m_stdout->readAll(),
             QByteArray("Title: Sample Entry\n"
                        "UserName: User Name\n"
                        "Password: PROTECTED\n"
                        "URL: http://www.somesite.com/\n"
                        "Notes: Notes\n"
                        "Uuid: {9f4544c2-ab00-c74a-8a1a-6eaf26cf57e9}\n"
                        "Tags: \n"
                        "TOTP Seed: PROTECTED\n"
                        "TOTP Settings: 30;6\n"
                        "TestAttribute1: b\n"
                        "testattribute1: a\n"));
}

void TestCli::testInvalidDbFiles()
{
    Show showCmd;
    QString nonExistentDbPath("/foo/bar/baz");
    QString directoryName("/");

    execCmd(showCmd, {"show", nonExistentDbPath, "/Sample Entry"});
    QCOMPARE(QString(m_stderr->readAll()),
             QObject::tr("Failed to open database file %1: not found").arg(nonExistentDbPath) + "\n");
    QCOMPARE(m_stdout->readAll(), QByteArray());

    execCmd(showCmd, {"show", directoryName, "whatever"});
    QCOMPARE(QString(m_stderr->readAll()),
             QObject::tr("Failed to open database file %1: not a plain file").arg(directoryName) + "\n");

    // Create a write-only file and try to open it.
    // QFileInfo.isReadable returns 'true' on Windows, even after the call to
    // setPermissions(WriteOwner) and with NTFS permissions enabled, so this
    // check doesn't work.
#if !defined(Q_OS_WIN)
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString path = QFileInfo(tempFile).absoluteFilePath();
    QVERIFY(tempFile.setPermissions(QFileDevice::WriteOwner));
    execCmd(showCmd, {"show", path, "some entry"});
    QCOMPARE(QString(m_stderr->readAll()),
             QObject::tr("Failed to open database file %1: not readable").arg(path) + "\n");
#endif // Q_OS_WIN
}

/**
 * Secret key for the YubiKey slot used by the unit test is
 * 1c e3 0f d7 8d 20 dc fa 40 b5 0c 18 77 9a fb 0f 02 28 8d b7
 * This secret can be on either slot but must be passive.
 */
void TestCli::testYubiKeyOption()
{
    if (!YubiKey::instance()->isInitialized()) {
        QSKIP("Unable to initialize YubiKey interface.");
    }

    YubiKey::instance()->findValidKeys();

    const auto keys = YubiKey::instance()->foundKeys().keys();
    if (keys.isEmpty()) {
        QSKIP("No YubiKey devices were detected.");
    }

    bool wouldBlock = false;
    QByteArray challenge("CLITest");
    Botan::secure_vector<char> response;
    QByteArray expected("\xA2\x3B\x94\x00\xBE\x47\x9A\x30\xA9\xEB\x50\x9B\x85\x56\x5B\x6B\x30\x25\xB4\x8E", 20);

    // Find a key that as configured for this test
    YubiKeySlot pKey(0, 0);
    for (auto key : keys) {
        if (YubiKey::instance()->testChallenge(key, &wouldBlock) && !wouldBlock) {
            YubiKey::instance()->challenge(key, challenge, response);
            if (std::memcmp(response.data(), expected.data(), expected.size()) == 0) {
                pKey = key;
                break;
            }
            Tools::wait(100);
        }
    }

    if (pKey.first == 0 && pKey.second == 0) {
        QSKIP("No YubiKey is properly configured to perform this test.");
    }

    List listCmd;
    Add addCmd;

    setInput("a");
    execCmd(listCmd,
            {"ls",
             "-y",
             QString("%1:%2").arg(QString::number(pKey.second), QString::number(pKey.first)),
             m_yubiKeyProtectedDbFile->fileName()});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll(), QByteArray());
    QCOMPARE(m_stdout->readAll(),
             QByteArray("entry1\n"
                        "entry2\n"));

    // Should raise an error with no yubikey slot.
    setInput("a");
    execCmd(listCmd, {"ls", m_yubiKeyProtectedDbFile->fileName()});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readLine(),
             QByteArray("Error while reading the database: Invalid credentials were provided, please try again.\n"));
    QCOMPARE(m_stderr->readLine(),
             QByteArray("If this reoccurs, then your database file may be corrupt. (HMAC mismatch)\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Should raise an error if yubikey slot is not a string
    setInput("a");
    execCmd(listCmd, {"ls", "-y", "invalidslot", m_yubiKeyProtectedDbFile->fileName()});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll().split(':').at(0), QByteArray("Invalid YubiKey slot invalidslot\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());

    // Should raise an error if yubikey slot is invalid.
    setInput("a");
    execCmd(listCmd, {"ls", "-y", "3", m_yubiKeyProtectedDbFile->fileName()});
    m_stderr->readLine(); // skip password prompt
    QCOMPARE(m_stderr->readAll().split(':').at(0), QByteArray("Invalid YubiKey slot 3\n"));
    QCOMPARE(m_stdout->readAll(), QByteArray());
}

void TestCli::testNonAscii()
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(
        KEEPASSX_CLI_PATH,
        QStringList(
            {"show", "-a", "password", m_nonAsciiDbFile->fileName(), QString::fromUtf8("\xe7\xa7\x98\xe5\xaf\x86")}));
    process.waitForStarted();
    QCOMPARE(process.state(), QProcess::ProcessState::Running);

    // Write password.
    process.write("\xce\x94\xc3\xb6\xd8\xb6\n");
    process.closeWriteChannel();

    process.waitForFinished();

    process.readLine(); // skip password prompt
    QByteArray password = process.readLine();
    QCOMPARE(QString::fromUtf8(password).trimmed(),
             QString::fromUtf8("\xf0\x9f\x9a\x97\xf0\x9f\x90\x8e\xf0\x9f\x94\x8b\xf0\x9f\x93\x8e"));
}

void TestCli::testTextStream()
{
    QFETCH(QString, codecName);
    QFETCH(QStringConverter::Encoding, expectedEncoding);

    // Set codec override via env var
    qputenv("ENCODING_OVERRIDE", codecName.toLatin1());

    TextStream stream;
    QCOMPARE(stream.encoding(), expectedEncoding);

    qunsetenv("ENCODING_OVERRIDE");
}

void TestCli::testTextStream_data()
{
    QTest::addColumn<QString>("codecName");
    QTest::addColumn<QStringConverter::Encoding>("expectedEncoding");

    QTest::newRow("UTF-8") << "UTF-8" << QStringConverter::Utf8;
    QTest::newRow("UTF-16") << "UTF-16" << QStringConverter::Utf16;
    QTest::newRow("UTF-16LE") << "UTF-16LE" << QStringConverter::Utf16LE;
    QTest::newRow("Utf16LE") << "Utf16LE" << QStringConverter::Utf16LE;
    QTest::newRow("UTF-32") << "UTF-32" << QStringConverter::Utf32;
    QTest::newRow("Utf32") << "Utf32" << QStringConverter::Utf32;
    QTest::newRow("UTF-32BE") << "UTF-32BE" << QStringConverter::Utf32BE;
    QTest::newRow("Utf32BE") << "Utf32BE" << QStringConverter::Utf32BE;
    QTest::newRow("UTF-32LE") << "UTF-32LE" << QStringConverter::Utf32LE;
    QTest::newRow("Utf32LE") << "Utf32LE" << QStringConverter::Utf32LE;
    QTest::newRow("ISO-8859-1") << "ISO-8859-1" << QStringConverter::Latin1;
    QTest::newRow("Latin1") << "Latin1" << QStringConverter::Latin1;
    QTest::newRow("Windows-850") << "Windows-850" << QStringConverter::System;
    QTest::newRow("Windows-1252") << "Windows-1252" << QStringConverter::System;
    QTest::newRow("System") << "System" << QStringConverter::System;
}

void TestCli::testCommandParsing_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QStringList>("expectedOutput");

    QTest::newRow("basic") << "hello world" << QStringList({"hello", "world"});
    QTest::newRow("basic escaping") << "hello\\ world" << QStringList({"hello world"});
    QTest::newRow("quoted string") << "\"hello world\"" << QStringList({"hello world"});
    QTest::newRow("multiple params") << "show Passwords/Internet" << QStringList({"show", "Passwords/Internet"});
    QTest::newRow("quoted string inside param")
        << R"(ls foo\ bar\ baz"quoted")" << QStringList({"ls", "foo bar baz\"quoted\""});
    QTest::newRow("multiple whitespace") << "hello    world" << QStringList({"hello", "world"});
    QTest::newRow("single slash char") << "\\" << QStringList({"\\"});
    QTest::newRow("double backslash entry name") << "show foo\\\\\\\\bar" << QStringList({"show", "foo\\\\bar"});
}

void TestCli::testCommandParsing()
{
    QFETCH(QString, input);
    QFETCH(QStringList, expectedOutput);

    QStringList result = Utils::splitCommandString(input);
    QCOMPARE(result.size(), expectedOutput.size());
    for (int i = 0; i < expectedOutput.size(); ++i) {
        QCOMPARE(result[i], expectedOutput[i]);
    }
}

void TestCli::testOpen()
{
    Open openCmd;

    setInput("a");
    execCmd(openCmd, {"open", m_dbFile->fileName()});
    QVERIFY(openCmd.currentDatabase);

    List listCmd;
    // Set a current database, simulating interactive mode.
    listCmd.currentDatabase = openCmd.currentDatabase;
    execCmd(listCmd, {"ls"});
    QByteArray expectedOutput("Sample Entry\n"
                              "General/\n"
                              "Windows/\n"
                              "Network/\n"
                              "Internet/\n"
                              "eMail/\n"
                              "Homebanking/\n");
    QByteArray actualOutput = m_stdout->readAll();
    actualOutput.truncate(expectedOutput.length());
    QCOMPARE(actualOutput, expectedOutput);
}

void TestCli::testHelp()
{
    Help helpCmd;
    Commands::setupCommands(false);

    execCmd(helpCmd, {"help"});
    QVERIFY(m_stdout->readAll().contains("Available commands"));

    List listCmd;
    execCmd(helpCmd, {"help", "ls"});
    QVERIFY(m_stdout->readAll().contains(listCmd.description.toLatin1()));
}
