/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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
#include "core/Global.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "keys/drivers/YubiKey.h"
#include "format/Kdbx3Reader.h"
#include "format/Kdbx3Writer.h"
#include "format/Kdbx4Reader.h"
#include "format/Kdbx4Writer.h"
#include "format/KdbxXmlReader.h"
#include "format/KeePass2.h"

#include "cli/Add.h"
#include "cli/AddGroup.h"
#include "cli/Analyze.h"
#include "cli/Clip.h"
#include "cli/Command.h"
#include "cli/Create.h"
#include "cli/Diceware.h"
#include "cli/Edit.h"
#include "cli/Estimate.h"
#include "cli/Export.h"
#include "cli/Generate.h"
#include "cli/Help.h"
#include "cli/Import.h"
#include "cli/List.h"
#include "cli/Locate.h"
#include "cli/Merge.h"
#include "cli/Move.h"
#include "cli/Open.h"
#include "cli/Remove.h"
#include "cli/RemoveGroup.h"
#include "cli/Show.h"
#include "cli/Utils.h"

#include <QClipboard>
#include <QFile>
#include <QFuture>
#include <QSet>
#include <QTextStream>
#include <QtConcurrent>

#include <cstdio>

QTEST_MAIN(TestCli)

QSharedPointer<Database> globalCurrentDatabase;

void TestCli::initTestCase()
{
    QVERIFY(Crypto::init());

    Config::createTempFileInstance();
    Bootstrap::bootstrapApplication();

    // Load the NewDatabase.kdbx file into temporary storage
    QFile sourceDbFile(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.kdbx"));
    QVERIFY(sourceDbFile.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFile, m_dbData));
    sourceDbFile.close();

    // Load the NewDatabase2.kdbx file into temporary storage
    QFile sourceDbFile2(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase2.kdbx"));
    QVERIFY(sourceDbFile2.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFile2, m_dbData2));
    sourceDbFile2.close();

    // Load the KeyFileProtected.kdbx file into temporary storage
    QFile sourceDbFile3(QString(KEEPASSX_TEST_DATA_DIR).append("/KeyFileProtected.kdbx"));
    QVERIFY(sourceDbFile3.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFile3, m_keyFileProtectedDbData));
    sourceDbFile3.close();

    // Load the KeyFileProtectedNoPassword.kdbx file into temporary storage
    QFile sourceDbFile4(QString(KEEPASSX_TEST_DATA_DIR).append("/KeyFileProtectedNoPassword.kdbx"));
    QVERIFY(sourceDbFile4.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFile4, m_keyFileProtectedNoPasswordDbData));
    sourceDbFile4.close();

    QFile sourceDbFileYubiKeyProtected(QString(KEEPASSX_TEST_DATA_DIR).append("/YubiKeyProtectedPasswords.kdbx"));
    QVERIFY(sourceDbFileYubiKeyProtected.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFileYubiKeyProtected, m_yubiKeyProtectedDbData));
    sourceDbFileYubiKeyProtected.close();

    // Load the NewDatabase.xml file into temporary storage
    QFile sourceXmlFile(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.xml"));
    QVERIFY(sourceXmlFile.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceXmlFile, m_xmlData));
    sourceXmlFile.close();
}

void TestCli::init()
{
    m_dbFile.reset(new TemporaryFile());
    m_dbFile->open();
    m_dbFile->write(m_dbData);
    m_dbFile->close();

    m_dbFile2.reset(new TemporaryFile());
    m_dbFile2->open();
    m_dbFile2->write(m_dbData2);
    m_dbFile2->close();

    m_xmlFile.reset(new TemporaryFile());
    m_xmlFile->open();
    m_xmlFile->write(m_xmlData);
    m_xmlFile->close();

    m_keyFileProtectedDbFile.reset(new TemporaryFile());
    m_keyFileProtectedDbFile->open();
    m_keyFileProtectedDbFile->write(m_keyFileProtectedDbData);
    m_keyFileProtectedDbFile->close();

    m_keyFileProtectedNoPasswordDbFile.reset(new TemporaryFile());
    m_keyFileProtectedNoPasswordDbFile->open();
    m_keyFileProtectedNoPasswordDbFile->write(m_keyFileProtectedNoPasswordDbData);
    m_keyFileProtectedNoPasswordDbFile->close();

    m_yubiKeyProtectedDbFile.reset(new TemporaryFile());
    m_yubiKeyProtectedDbFile->open();
    m_yubiKeyProtectedDbFile->write(m_yubiKeyProtectedDbData);
    m_yubiKeyProtectedDbFile->close();

    m_stdinFile.reset(new TemporaryFile());
    m_stdinFile->open();
    Utils::STDIN = fdopen(m_stdinFile->handle(), "r+");

    m_stdoutFile.reset(new TemporaryFile());
    m_stdoutFile->open();
    Utils::STDOUT = fdopen(m_stdoutFile->handle(), "r+");

    m_stderrFile.reset(new TemporaryFile());
    m_stderrFile->open();
    Utils::STDERR = fdopen(m_stderrFile->handle(), "r+");
}

void TestCli::cleanup()
{
    m_dbFile.reset();
    m_dbFile2.reset();
    m_keyFileProtectedDbFile.reset();
    m_keyFileProtectedNoPasswordDbFile.reset();
    m_yubiKeyProtectedDbFile.reset();

    m_stdinFile.reset();
    Utils::STDIN = stdin;

    m_stdoutFile.reset();
    Utils::STDOUT = stdout;

    m_stderrFile.reset();
    Utils::STDERR = stderr;
}

void TestCli::cleanupTestCase()
{
}

QSharedPointer<Database> TestCli::readTestDatabase() const
{
    Utils::Test::setNextPassword("a");
    auto db = QSharedPointer<Database>(Utils::unlockDatabase(m_dbFile->fileName(), true, "", "", Utils::STDOUT));
    m_stdoutFile->seek(ftell(Utils::STDOUT)); // re-synchronize handles
    return db;
}

void TestCli::testBatchCommands()
{
    Commands::setupCommands(false);
    QVERIFY(Commands::getCommand("add"));
    QVERIFY(Commands::getCommand("analyze"));
    QVERIFY(Commands::getCommand("clip"));
    QVERIFY(Commands::getCommand("close"));
    QVERIFY(Commands::getCommand("create"));
    QVERIFY(Commands::getCommand("diceware"));
    QVERIFY(Commands::getCommand("edit"));
    QVERIFY(Commands::getCommand("estimate"));
    QVERIFY(Commands::getCommand("export"));
    QVERIFY(Commands::getCommand("generate"));
    QVERIFY(Commands::getCommand("help"));
    QVERIFY(Commands::getCommand("import"));
    QVERIFY(Commands::getCommand("locate"));
    QVERIFY(Commands::getCommand("ls"));
    QVERIFY(Commands::getCommand("merge"));
    QVERIFY(Commands::getCommand("mkdir"));
    QVERIFY(Commands::getCommand("mv"));
    QVERIFY(Commands::getCommand("open"));
    QVERIFY(Commands::getCommand("rm"));
    QVERIFY(Commands::getCommand("rmdir"));
    QVERIFY(Commands::getCommand("show"));
    QVERIFY(!Commands::getCommand("doesnotexist"));
    QCOMPARE(Commands::getCommands().size(), 21);
}

void TestCli::testInteractiveCommands()
{
    Commands::setupCommands(true);
    QVERIFY(Commands::getCommand("add"));
    QVERIFY(Commands::getCommand("analyze"));
    QVERIFY(Commands::getCommand("clip"));
    QVERIFY(Commands::getCommand("close"));
    QVERIFY(Commands::getCommand("create"));
    QVERIFY(Commands::getCommand("diceware"));
    QVERIFY(Commands::getCommand("edit"));
    QVERIFY(Commands::getCommand("estimate"));
    QVERIFY(Commands::getCommand("exit"));
    QVERIFY(Commands::getCommand("generate"));
    QVERIFY(Commands::getCommand("help"));
    QVERIFY(Commands::getCommand("locate"));
    QVERIFY(Commands::getCommand("ls"));
    QVERIFY(Commands::getCommand("merge"));
    QVERIFY(Commands::getCommand("mkdir"));
    QVERIFY(Commands::getCommand("mv"));
    QVERIFY(Commands::getCommand("open"));
    QVERIFY(Commands::getCommand("quit"));
    QVERIFY(Commands::getCommand("rm"));
    QVERIFY(Commands::getCommand("rmdir"));
    QVERIFY(Commands::getCommand("show"));
    QVERIFY(!Commands::getCommand("doesnotexist"));
    QCOMPARE(Commands::getCommands().size(), 21);
}

void TestCli::testAdd()
{
    Add addCmd;
    QVERIFY(!addCmd.name.isEmpty());
    QVERIFY(addCmd.getDescriptionLine().contains(addCmd.name));

    Utils::Test::setNextPassword("a");
    addCmd.execute({"add",
                    "-u",
                    "newuser",
                    "--url",
                    "https://example.com/",
                    "-g",
                    "-L",
                    "20",
                    m_dbFile->fileName(),
                    "/newuser-entry"});
    m_stderrFile->reset();
    m_stdoutFile->reset();
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully added entry newuser-entry.\n"));

    auto db = readTestDatabase();
    auto* entry = db->rootGroup()->findEntryByPath("/newuser-entry");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://example.com/"));
    QCOMPARE(entry->password().size(), 20);

    // Quiet option
    qint64 pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    addCmd.execute({"add", "-q", "-u", "newuser", "-g", "-L", "20", m_dbFile->fileName(), "/newentry-quiet"});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/newentry-quiet");
    QVERIFY(entry);
    QCOMPARE(entry->password().size(), 20);

    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    Utils::Test::setNextPassword("newpassword");
    addCmd.execute(
        {"add", "-u", "newuser2", "--url", "https://example.net/", "-p", m_dbFile->fileName(), "/newuser-entry2"});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    m_stdoutFile->readLine(); // skip password prompt
    m_stdoutFile->readLine(); // skip password input
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully added entry newuser-entry2.\n"));

    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/newuser-entry2");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser2"));
    QCOMPARE(entry->url(), QString("https://example.net/"));
    QCOMPARE(entry->password(), QString("newpassword"));

    // Password generation options
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    addCmd.execute({"add", "-u", "newuser3", "-g", "-L", "34", m_dbFile->fileName(), "/newuser-entry3"});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully added entry newuser-entry3.\n"));

    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/newuser-entry3");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser3"));
    QCOMPARE(entry->password().size(), 34);
    QRegularExpression defaultPasswordClassesRegex("^[a-zA-Z0-9]+$");
    QVERIFY(defaultPasswordClassesRegex.match(entry->password()).hasMatch());

    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    addCmd.execute({"add",
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
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully added entry newuser-entry4.\n"));

    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/newuser-entry4");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser4"));
    QCOMPARE(entry->password().size(), 20);
    QVERIFY(!defaultPasswordClassesRegex.match(entry->password()).hasMatch());
}

void TestCli::testAddGroup()
{
    AddGroup addGroupCmd;
    QVERIFY(!addGroupCmd.name.isEmpty());
    QVERIFY(addGroupCmd.getDescriptionLine().contains(addGroupCmd.name));

    Utils::Test::setNextPassword("a");
    addGroupCmd.execute({"mkdir", m_dbFile->fileName(), "/new_group"});
    m_stderrFile->reset();
    m_stdoutFile->reset();
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully added group new_group.\n"));

    auto db = readTestDatabase();
    auto* group = db->rootGroup()->findGroupByPath("new_group");
    QVERIFY(group);
    QCOMPARE(group->name(), QString("new_group"));

    // Trying to add the same group should fail.
    qint64 pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    addGroupCmd.execute({"mkdir", m_dbFile->fileName(), "/new_group"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Group /new_group already exists!\n"));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));

    // Should be able to add groups down the tree.
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    addGroupCmd.execute({"mkdir", m_dbFile->fileName(), "/new_group/newer_group"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully added group newer_group.\n"));

    db = readTestDatabase();
    group = db->rootGroup()->findGroupByPath("new_group/newer_group");
    QVERIFY(group);
    QCOMPARE(group->name(), QString("newer_group"));

    // Should fail if the path is invalid.
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    addGroupCmd.execute({"mkdir", m_dbFile->fileName(), "/invalid_group/newer_group"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Group /invalid_group not found.\n"));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));

    // Should fail to add the root group.
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    addGroupCmd.execute({"mkdir", m_dbFile->fileName(), "/"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Group / already exists!\n"));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
}

bool isTOTP(const QString& value)
{
    QString val = value.trimmed();
    if (val.length() < 5 || val.length() > 6) {
        return false;
    }
    for (int i = 0; i < val.length(); ++i) {
        if (!value[i].isDigit()) {
            return false;
        }
    }
    return true;
}

void TestCli::testAnalyze()
{
    Analyze analyzeCmd;
    QVERIFY(!analyzeCmd.name.isEmpty());
    QVERIFY(analyzeCmd.getDescriptionLine().contains(analyzeCmd.name));

    const QString hibpPath = QString(KEEPASSX_TEST_DATA_DIR).append("/hibp.txt");

    Utils::Test::setNextPassword("a");
    analyzeCmd.execute({"analyze", "--hibp", hibpPath, m_dbFile->fileName()});
    m_stdoutFile->reset();
    m_stdoutFile->readLine(); // skip password prompt
    auto output = m_stdoutFile->readAll();
    QVERIFY(output.contains("Sample Entry") && output.contains("123"));
}

void TestCli::testClip()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->clear();

    Clip clipCmd;
    QVERIFY(!clipCmd.name.isEmpty());
    QVERIFY(clipCmd.getDescriptionLine().contains(clipCmd.name));

    // Password
    Utils::Test::setNextPassword("a");
    clipCmd.execute({"clip", m_dbFile->fileName(), "/Sample Entry"});

    m_stderrFile->reset();
    m_stdoutFile->reset();
    QString errorOutput(m_stderrFile->readAll());

    if (errorOutput.contains("Unable to start program")
        || errorOutput.contains("No program defined for clipboard manipulation")) {
        QSKIP("Clip test skipped due to missing clipboard tool");
    }

    QCOMPARE(clipboard->text(), QString("Password"));
    m_stdoutFile->readLine(); // skip prompt line
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Entry's password copied to the clipboard!\n"));

    // Quiet option
    qint64 pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    clipCmd.execute({"clip", m_dbFile->fileName(), "/Sample Entry", "-q"});
    m_stdoutFile->seek(pos);
    // Output should be empty when quiet option is set.
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(clipboard->text(), QString("Password"));

    // TOTP
    Utils::Test::setNextPassword("a");
    clipCmd.execute({"clip", m_dbFile->fileName(), "/Sample Entry", "--totp"});

    QVERIFY(isTOTP(clipboard->text()));

    // Password with timeout
    Utils::Test::setNextPassword("a");
    // clang-format off
    QFuture<void> future = QtConcurrent::run(&clipCmd,
                                             static_cast<int(Clip::*)(const QStringList&)>(&DatabaseCommand::execute),
                                             QStringList{"clip", m_dbFile->fileName(), "/Sample Entry", "1"});
    // clang-format on

    QTRY_COMPARE_WITH_TIMEOUT(clipboard->text(), QString("Password"), 500);
    QTRY_COMPARE_WITH_TIMEOUT(clipboard->text(), QString(""), 1500);

    future.waitForFinished();

    // TOTP with timeout
    Utils::Test::setNextPassword("a");
    future = QtConcurrent::run(&clipCmd,
                               static_cast<int (Clip::*)(const QStringList&)>(&DatabaseCommand::execute),
                               QStringList{"clip", m_dbFile->fileName(), "/Sample Entry", "1", "-t"});

    QTRY_VERIFY_WITH_TIMEOUT(isTOTP(clipboard->text()), 500);
    QTRY_COMPARE_WITH_TIMEOUT(clipboard->text(), QString(""), 1500);

    future.waitForFinished();

    qint64 posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    clipCmd.execute({"clip", m_dbFile->fileName(), "--totp", "/Sample Entry", "0"});
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Invalid timeout value 0.\n"));

    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    clipCmd.execute({"clip", m_dbFile->fileName(), "--totp", "/Sample Entry", "bleuh"});
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Invalid timeout value bleuh.\n"));

    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    clipCmd.execute({"clip", m_dbFile2->fileName(), "--totp", "/Sample Entry"});
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Entry with path /Sample Entry has no TOTP set up.\n"));
}

void TestCli::testCreate()
{
    Create createCmd;
    QVERIFY(!createCmd.name.isEmpty());
    QVERIFY(createCmd.getDescriptionLine().contains(createCmd.name));

    QScopedPointer<QTemporaryDir> testDir(new QTemporaryDir());

    QString databaseFilename = testDir->path() + "/testCreate1.kdbx";
    // Password
    Utils::Test::setNextPassword("a");
    createCmd.execute({"create", databaseFilename});

    m_stderrFile->reset();
    m_stdoutFile->reset();

    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Successfully created new database.\n"));

    Utils::Test::setNextPassword("a");
    auto db = QSharedPointer<Database>(Utils::unlockDatabase(databaseFilename, true, "", "", Utils::DEVNULL));
    QVERIFY(db);

    // Should refuse to create the database if it already exists.
    qint64 pos = m_stdoutFile->pos();
    qint64 errPos = m_stderrFile->pos();
    createCmd.execute({"create", databaseFilename});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(errPos);
    // Output should be empty when there is an error.
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QString errorMessage = QString("File " + databaseFilename + " already exists.\n");
    QCOMPARE(m_stderrFile->readAll(), errorMessage.toUtf8());

    // Testing with keyfile creation
    QString databaseFilename2 = testDir->path() + "/testCreate2.kdbx";
    QString keyfilePath = testDir->path() + "/keyfile.txt";
    pos = m_stdoutFile->pos();
    errPos = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    createCmd.execute({"create", databaseFilename2, "-k", keyfilePath});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(errPos);

    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Successfully created new database.\n"));

    Utils::Test::setNextPassword("a");
    auto db2 =
        QSharedPointer<Database>(Utils::unlockDatabase(databaseFilename2, true, keyfilePath, "", Utils::DEVNULL));
    QVERIFY(db2);

    // Testing with existing keyfile
    QString databaseFilename3 = testDir->path() + "/testCreate3.kdbx";
    pos = m_stdoutFile->pos();
    errPos = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    createCmd.execute({"create", databaseFilename3, "-k", keyfilePath});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(errPos);

    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Successfully created new database.\n"));

    Utils::Test::setNextPassword("a");
    auto db3 =
        QSharedPointer<Database>(Utils::unlockDatabase(databaseFilename3, true, keyfilePath, "", Utils::DEVNULL));
    QVERIFY(db3);
}

void TestCli::testDiceware()
{
    Diceware dicewareCmd;
    QVERIFY(!dicewareCmd.name.isEmpty());
    QVERIFY(dicewareCmd.getDescriptionLine().contains(dicewareCmd.name));

    dicewareCmd.execute({"diceware"});
    m_stdoutFile->reset();
    QString passphrase(m_stdoutFile->readLine());
    QVERIFY(!passphrase.isEmpty());

    dicewareCmd.execute({"diceware", "-W", "2"});
    m_stdoutFile->seek(passphrase.toLatin1().size());
    passphrase = m_stdoutFile->readLine();
    QCOMPARE(passphrase.split(" ").size(), 2);

    auto pos = m_stdoutFile->pos();
    dicewareCmd.execute({"diceware", "-W", "10"});
    m_stdoutFile->seek(pos);
    passphrase = m_stdoutFile->readLine();
    QCOMPARE(passphrase.split(" ").size(), 10);

    // Testing with invalid word count
    auto posErr = m_stderrFile->pos();
    dicewareCmd.execute({"diceware", "-W", "-10"});
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readLine(), QByteArray("Invalid word count -10\n"));

    // Testing with invalid word count format
    posErr = m_stderrFile->pos();
    dicewareCmd.execute({"diceware", "-W", "bleuh"});
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readLine(), QByteArray("Invalid word count bleuh\n"));

    TemporaryFile wordFile;
    wordFile.open();
    for (int i = 0; i < 4500; ++i) {
        wordFile.write(QString("word" + QString::number(i) + "\n").toLatin1());
    }
    wordFile.close();

    pos = m_stdoutFile->pos();
    dicewareCmd.execute({"diceware", "-W", "11", "-w", wordFile.fileName()});
    m_stdoutFile->seek(pos);
    passphrase = m_stdoutFile->readLine();
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

    posErr = m_stderrFile->pos();
    dicewareCmd.execute({"diceware", "-W", "11", "-w", smallWordFile.fileName()});
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readLine(), QByteArray("The word list is too small (< 1000 items)\n"));
}

void TestCli::testEdit()
{
    Edit editCmd;
    QVERIFY(!editCmd.name.isEmpty());
    QVERIFY(editCmd.getDescriptionLine().contains(editCmd.name));

    Utils::Test::setNextPassword("a");
    // clang-format off
    editCmd.execute({"edit", "-u", "newuser", "--url", "https://otherurl.example.com/", "-t", "newtitle", m_dbFile->fileName(), "/Sample Entry"});
    // clang-format on
    m_stdoutFile->reset();
    m_stdoutFile->readLine(); // skip prompt line
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Successfully edited entry newtitle.\n"));

    auto db = readTestDatabase();
    auto* entry = db->rootGroup()->findEntryByPath("/newtitle");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://otherurl.example.com/"));
    QCOMPARE(entry->password(), QString("Password"));

    // Quiet option
    qint64 pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    editCmd.execute({"edit", m_dbFile->fileName(), "-q", "-t", "newertitle", "/newtitle"});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));

    Utils::Test::setNextPassword("a");
    editCmd.execute({"edit", "-g", m_dbFile->fileName(), "/newertitle"});
    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/newertitle");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://otherurl.example.com/"));
    QVERIFY(!entry->password().isEmpty());
    QVERIFY(entry->password() != QString("Password"));

    Utils::Test::setNextPassword("a");
    editCmd.execute({"edit", "-g", "-L", "34", "-t", "evennewertitle", m_dbFile->fileName(), "/newertitle"});
    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/evennewertitle");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://otherurl.example.com/"));
    QVERIFY(entry->password() != QString("Password"));
    QCOMPARE(entry->password().size(), 34);
    QRegularExpression defaultPasswordClassesRegex("^[a-zA-Z0-9]+$");
    QVERIFY(defaultPasswordClassesRegex.match(entry->password()).hasMatch());

    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    editCmd.execute({"edit",
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
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully edited entry evennewertitle.\n"));

    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/evennewertitle");
    QVERIFY(entry);
    QCOMPARE(entry->password().size(), 20);
    QVERIFY(!defaultPasswordClassesRegex.match(entry->password()).hasMatch());

    Utils::Test::setNextPassword("a");
    Utils::Test::setNextPassword("newpassword");
    editCmd.execute({"edit", "-p", m_dbFile->fileName(), "/evennewertitle"});
    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/evennewertitle");
    QVERIFY(entry);
    QCOMPARE(entry->password(), QString("newpassword"));
}

void TestCli::testEstimate_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("length");
    QTest::addColumn<QString>("entropy");
    QTest::addColumn<QString>("log10");
    QTest::addColumn<QStringList>("searchStrings");

    QTest::newRow("Dictionary") << "password"
                                << "8"
                                << "1.0"
                                << "0.3" << QStringList{"Type: Dictionary", "\tpassword"};

    QTest::newRow("Spatial") << "zxcv"
                             << "4"
                             << "10.3"
                             << "3.1" << QStringList{"Type: Spatial", "\tzxcv"};

    QTest::newRow("Spatial(Rep)") << "sdfgsdfg"
                                  << "8"
                                  << "11.3"
                                  << "3.4" << QStringList{"Type: Spatial(Rep)", "\tsdfgsdfg"};

    QTest::newRow("Dictionary / Sequence")
        << "password123"
        << "11"
        << "4.5"
        << "1.3" << QStringList{"Type: Dictionary", "Type: Sequence", "\tpassword", "\t123"};

    QTest::newRow("Dict+Leet") << "p455w0rd"
                               << "8"
                               << "2.5"
                               << "0.7" << QStringList{"Type: Dict+Leet", "\tp455w0rd"};

    QTest::newRow("Dictionary(Rep)") << "hellohello"
                                     << "10"
                                     << "7.3"
                                     << "2.2" << QStringList{"Type: Dictionary(Rep)", "\thellohello"};

    QTest::newRow("Sequence(Rep) / Dictionary")
        << "456456foobar"
        << "12"
        << "16.7"
        << "5.0" << QStringList{"Type: Sequence(Rep)", "Type: Dictionary", "\t456456", "\tfoobar"};

    QTest::newRow("Bruteforce(Rep) / Bruteforce")
        << "xzxzy"
        << "5"
        << "16.1"
        << "4.8" << QStringList{"Type: Bruteforce(Rep)", "Type: Bruteforce", "\txzxz", "\ty"};

    QTest::newRow("Dictionary / Date(Rep)")
        << "pass20182018"
        << "12"
        << "15.1"
        << "4.56" << QStringList{"Type: Dictionary", "Type: Date(Rep)", "\tpass", "\t20182018"};

    QTest::newRow("Dictionary / Date / Bruteforce")
        << "mypass2018-2"
        << "12"
        << "32.9"
        << "9.9" << QStringList{"Type: Dictionary", "Type: Date", "Type: Bruteforce", "\tmypass", "\t2018", "\t-2"};

    QTest::newRow("Strong Password") << "E*!%.Qw{t.X,&bafw)\"Q!ah$%;U/"
                                     << "28"
                                     << "165.7"
                                     << "49.8" << QStringList{"Type: Bruteforce", "\tE*"};

    // TODO: detect passphrases and adjust entropy calculation accordingly (issue #2347)
    QTest::newRow("Strong Passphrase")
        << "squint wooing resupply dangle isolation axis headsman"
        << "53"
        << "151.2"
        << "45.5"
        << QStringList{
               "Type: Dictionary", "Type: Bruteforce", "Multi-word extra bits 22.0", "\tsquint", "\t ", "\twooing"};
}

void TestCli::testEstimate()
{
    QFETCH(QString, input);
    QFETCH(QString, length);
    QFETCH(QString, entropy);
    QFETCH(QString, log10);
    QFETCH(QStringList, searchStrings);

    Estimate estimateCmd;
    QVERIFY(!estimateCmd.name.isEmpty());
    QVERIFY(estimateCmd.getDescriptionLine().contains(estimateCmd.name));

    QTextStream in(m_stdinFile.data());
    QTextStream out(m_stdoutFile.data());

    in << input << endl;
    in.seek(0);
    estimateCmd.execute({"estimate", "-a"});
    out.seek(0);
    auto result = out.readAll();
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

    Utils::Test::setNextPassword("a");
    exportCmd.execute({"export", m_dbFile->fileName()});

    m_stdoutFile->seek(0);
    m_stdoutFile->readLine(); // skip prompt line

    KdbxXmlReader reader(KeePass2::FILE_VERSION_3_1);
    QScopedPointer<Database> db(new Database());
    reader.readDatabase(m_stdoutFile.data(), db.data());
    QVERIFY(!reader.hasError());
    QVERIFY(db.data());
    auto* entry = db->rootGroup()->findEntryByPath("/Sample Entry");
    QVERIFY(entry);
    QCOMPARE(entry->password(), QString("Password"));

    m_stdoutFile->reset();

    // Quiet option
    QScopedPointer<Database> dbQuiet(new Database());
    qint64 pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    exportCmd.execute({"export", "-f", "xml", "-q", m_dbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    reader.readDatabase(m_stdoutFile.data(), dbQuiet.data());
    QVERIFY(!reader.hasError());
    QVERIFY(db.data());
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));

    // CSV exporting
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    exportCmd.execute({"export", "-f", "csv", m_dbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip prompt line
    m_stderrFile->seek(posErr);
    QByteArray csvHeader = m_stdoutFile->readLine();
    QCOMPARE(csvHeader, QByteArray("\"Group\",\"Title\",\"Username\",\"Password\",\"URL\",\"Notes\"\n"));
    QByteArray csvData = m_stdoutFile->readAll();
    QVERIFY(csvData.contains(QByteArray(
        "\"NewDatabase\",\"Sample Entry\",\"User Name\",\"Password\",\"http://www.somesite.com/\",\"Notes\"\n")));
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));

    // test invalid format
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    exportCmd.execute({"export", "-f", "yaml", m_dbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip prompt line
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readLine(), QByteArray(""));
    QCOMPARE(m_stderrFile->readLine(), QByteArray("Unsupported format yaml\n"));
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
}

void TestCli::testGenerate()
{
    QFETCH(QStringList, parameters);
    QFETCH(QString, pattern);

    Generate generateCmd;
    QVERIFY(!generateCmd.name.isEmpty());
    QVERIFY(generateCmd.getDescriptionLine().contains(generateCmd.name));

    qint64 pos = 0;
    // run multiple times to make accidental passes unlikely
    TextStream stream(m_stdoutFile.data());
    for (int i = 0; i < 10; ++i) {
        generateCmd.execute(parameters);
        stream.seek(pos);
        QRegularExpression regex(pattern);
        QString password = stream.readLine();
        pos = stream.pos();
        QVERIFY2(regex.match(password).hasMatch(),
                 qPrintable("Password " + password + " does not match pattern " + pattern));
    }

    // Testing with invalid password length
    auto posErr = m_stderrFile->pos();
    generateCmd.execute({"generate", "-L", "-10"});
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readLine(), QByteArray("Invalid password length -10\n"));

    posErr = m_stderrFile->pos();
    generateCmd.execute({"generate", "-L", "0"});
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readLine(), QByteArray("Invalid password length 0\n"));

    // Testing with invalid word count format
    posErr = m_stderrFile->pos();
    generateCmd.execute({"generate", "-L", "bleuh"});
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readLine(), QByteArray("Invalid password length bleuh\n"));
}

void TestCli::testImport()
{
    Import importCmd;
    QVERIFY(!importCmd.name.isEmpty());
    QVERIFY(importCmd.getDescriptionLine().contains(importCmd.name));

    QScopedPointer<QTemporaryDir> testDir(new QTemporaryDir());
    QString databaseFilename = testDir->path() + "testImport1.kdbx";

    Utils::Test::setNextPassword("a");
    importCmd.execute({"import", m_xmlFile->fileName(), databaseFilename});

    m_stderrFile->reset();
    m_stdoutFile->reset();

    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Enter password to encrypt database (optional): \n"));
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Successfully imported database.\n"));

    Utils::Test::setNextPassword("a");
    auto db = QSharedPointer<Database>(Utils::unlockDatabase(databaseFilename, true, "", "", Utils::DEVNULL));
    QVERIFY(db);
    auto* entry = db->rootGroup()->findEntryByPath("/Sample Entry 1");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("User Name"));

    // Should refuse to create the database if it already exists.
    qint64 pos = m_stdoutFile->pos();
    qint64 errPos = m_stderrFile->pos();
    importCmd.execute({"import", m_xmlFile->fileName(), databaseFilename});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(errPos);
    // Output should be empty when there is an error.
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QString errorMessage = QString("File " + databaseFilename + " already exists.\n");
    QCOMPARE(m_stderrFile->readAll(), errorMessage.toUtf8());

    // Quiet option
    QScopedPointer<QTemporaryDir> testDirQuiet(new QTemporaryDir());
    QString databaseFilenameQuiet = testDirQuiet->path() + "testImport2.kdbx";

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    importCmd.execute({"import", "-q", m_xmlFile->fileName(), databaseFilenameQuiet});
    m_stdoutFile->seek(pos);

    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Enter password to encrypt database (optional): \n"));

    Utils::Test::setNextPassword("a");
    auto dbQuiet = QSharedPointer<Database>(Utils::unlockDatabase(databaseFilenameQuiet, true, "", "", Utils::DEVNULL));
    QVERIFY(dbQuiet);
}

void TestCli::testKeyFileOption()
{
    List listCmd;

    QString keyFilePath(QString(KEEPASSX_TEST_DATA_DIR).append("/KeyFileProtected.key"));
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", "-k", keyFilePath, m_keyFileProtectedDbFile->fileName()});
    m_stdoutFile->reset();
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("entry1\n"
                        "entry2\n"));

    // Should raise an error with no key file.
    qint64 pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", m_keyFileProtectedDbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QVERIFY(m_stderrFile->readAll().contains("Invalid credentials were provided"));

    // Should raise an error if key file path is invalid.
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", "-k", "invalidpath", m_keyFileProtectedDbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll().split(':').at(0), QByteArray("Failed to load key file invalidpath"));
}

void TestCli::testNoPasswordOption()
{
    List listCmd;

    QString keyFilePath(QString(KEEPASSX_TEST_DATA_DIR).append("/KeyFileProtectedNoPassword.key"));
    listCmd.execute({"ls", "-k", keyFilePath, "--no-password", m_keyFileProtectedNoPasswordDbFile->fileName()});
    m_stdoutFile->reset();
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("entry1\n"
                        "entry2\n"));

    // Should raise an error with no key file.
    qint64 pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();
    listCmd.execute({"ls", "--no-password", m_keyFileProtectedNoPasswordDbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QVERIFY(m_stderrFile->readAll().contains("Invalid credentials were provided"));
}

void TestCli::testList()
{
    List listCmd;
    QVERIFY(!listCmd.name.isEmpty());
    QVERIFY(listCmd.getDescriptionLine().contains(listCmd.name));

    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", m_dbFile->fileName()});
    m_stdoutFile->reset();
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("Sample Entry\n"
                        "General/\n"
                        "Windows/\n"
                        "Network/\n"
                        "Internet/\n"
                        "eMail/\n"
                        "Homebanking/\n"));

    // Quiet option
    qint64 pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", "-q", m_dbFile->fileName()});
    m_stdoutFile->seek(pos);
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("Sample Entry\n"
                        "General/\n"
                        "Windows/\n"
                        "Network/\n"
                        "Internet/\n"
                        "eMail/\n"
                        "Homebanking/\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", "-R", m_dbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(),
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

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", "-R", "-f", m_dbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(),
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

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", "-R", "-f", m_dbFile->fileName(), "/Homebanking"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("Subgroup/\n"
                        "Subgroup/Subgroup Entry\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", m_dbFile->fileName(), "/General/"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine();
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("[empty]\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", m_dbFile->fileName(), "/DoesNotExist/"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->reset();
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Cannot find group /DoesNotExist/.\n"));
}

void TestCli::testLocate()
{
    Locate locateCmd;
    QVERIFY(!locateCmd.name.isEmpty());
    QVERIFY(locateCmd.getDescriptionLine().contains(locateCmd.name));

    Utils::Test::setNextPassword("a");
    locateCmd.execute({"locate", m_dbFile->fileName(), "Sample"});
    m_stdoutFile->reset();
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("/Sample Entry\n"));

    // Quiet option
    qint64 pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    locateCmd.execute({"locate", m_dbFile->fileName(), "-q", "Sample"});
    m_stdoutFile->seek(pos);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("/Sample Entry\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    locateCmd.execute({"locate", m_dbFile->fileName(), "Does Not Exist"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->reset();
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll(), QByteArray("No results for that search term.\n"));

    // write a modified database
    auto db = readTestDatabase();
    QVERIFY(db);
    auto* group = db->rootGroup()->findGroupByPath("/General/");
    QVERIFY(group);
    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setTitle("New Entry");
    group->addEntry(entry);
    TemporaryFile tmpFile;
    tmpFile.open();
    Kdbx4Writer writer;
    writer.writeDatabase(&tmpFile, db.data());
    tmpFile.close();

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    locateCmd.execute({"locate", tmpFile.fileName(), "New"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("/General/New Entry\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    locateCmd.execute({"locate", tmpFile.fileName(), "Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("/Sample Entry\n/General/New Entry\n/Homebanking/Subgroup/Subgroup Entry\n"));
}

void TestCli::testMerge()
{
    Merge mergeCmd;
    QVERIFY(!mergeCmd.name.isEmpty());
    QVERIFY(mergeCmd.getDescriptionLine().contains(mergeCmd.name));

    Kdbx4Writer writer;
    Kdbx4Reader reader;

    // load test database and save copies
    auto db = readTestDatabase();
    QVERIFY(db);
    TemporaryFile targetFile1;
    targetFile1.open();
    writer.writeDatabase(&targetFile1, db.data());
    targetFile1.close();
    TemporaryFile targetFile2;
    targetFile2.open();
    writer.writeDatabase(&targetFile2, db.data());
    targetFile2.close();

    // save another copy with a different password
    TemporaryFile targetFile3;
    targetFile3.open();
    auto oldKey = db->key();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("b"));
    db->setKey(key);
    writer.writeDatabase(&targetFile3, db.data());
    targetFile3.close();
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
    writer.writeDatabase(&sourceFile, db.data());
    sourceFile.close();

    qint64 pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    mergeCmd.execute({"merge", "-s", targetFile1.fileName(), sourceFile.fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine();
    m_stderrFile->reset();
    QList<QByteArray> outLines1 = m_stdoutFile->readAll().split('\n');
    QCOMPARE(outLines1.at(0).split('[').at(0), QByteArray("\tOverwriting Internet "));
    QCOMPARE(outLines1.at(1).split('[').at(0), QByteArray("\tCreating missing Some Website "));
    QCOMPARE(outLines1.at(2),
             QString("Successfully merged %1 into %2.").arg(sourceFile.fileName(), targetFile1.fileName()).toUtf8());

    QFile readBack(targetFile1.fileName());
    readBack.open(QIODevice::ReadOnly);
    auto mergedDb = QSharedPointer<Database>::create();
    reader.readDatabase(&readBack, oldKey, mergedDb.data());
    readBack.close();
    QVERIFY(mergedDb);
    auto* entry1 = mergedDb->rootGroup()->findEntryByPath("/Internet/Some Website");
    QVERIFY(entry1);
    QCOMPARE(entry1->title(), QString("Some Website"));
    QCOMPARE(entry1->password(), QString("secretsecretsecret"));

    // the dry run option should not modify the target database.
    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    mergeCmd.execute({"merge", "--dry-run", "-s", targetFile2.fileName(), sourceFile.fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine();
    m_stderrFile->reset();
    QList<QByteArray> outLines2 = m_stdoutFile->readAll().split('\n');
    QCOMPARE(outLines2.at(0).split('[').at(0), QByteArray("\tOverwriting Internet "));
    QCOMPARE(outLines2.at(1).split('[').at(0), QByteArray("\tCreating missing Some Website "));
    QCOMPARE(outLines2.at(2), QByteArray("Database was not modified by merge operation."));

    QFile readBack2(targetFile2.fileName());
    readBack2.open(QIODevice::ReadOnly);
    mergedDb = QSharedPointer<Database>::create();
    reader.readDatabase(&readBack2, oldKey, mergedDb.data());
    readBack2.close();
    QVERIFY(mergedDb);
    entry1 = mergedDb->rootGroup()->findEntryByPath("/Internet/Some Website");
    QVERIFY(!entry1);

    // the dry run option can be used with the quiet option
    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    mergeCmd.execute({"merge", "--dry-run", "-s", "-q", targetFile2.fileName(), sourceFile.fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine();
    m_stderrFile->reset();
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));

    readBack2.setFileName(targetFile2.fileName());
    readBack2.open(QIODevice::ReadOnly);
    mergedDb = QSharedPointer<Database>::create();
    reader.readDatabase(&readBack2, oldKey, mergedDb.data());
    readBack2.close();
    QVERIFY(mergedDb);
    entry1 = mergedDb->rootGroup()->findEntryByPath("/Internet/Some Website");
    QVERIFY(!entry1);

    // try again with different passwords for both files
    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("b");
    Utils::Test::setNextPassword("a");
    mergeCmd.execute({"merge", targetFile3.fileName(), sourceFile.fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine();
    m_stdoutFile->readLine();
    QList<QByteArray> outLines3 = m_stdoutFile->readAll().split('\n');
    QCOMPARE(outLines3.at(2),
             QString("Successfully merged %1 into %2.").arg(sourceFile.fileName(), targetFile3.fileName()).toUtf8());

    readBack.setFileName(targetFile3.fileName());
    readBack.open(QIODevice::ReadOnly);
    mergedDb = QSharedPointer<Database>::create();
    reader.readDatabase(&readBack, key, mergedDb.data());
    readBack.close();
    QVERIFY(mergedDb);
    entry1 = mergedDb->rootGroup()->findEntryByPath("/Internet/Some Website");
    QVERIFY(entry1);
    QCOMPARE(entry1->title(), QString("Some Website"));
    QCOMPARE(entry1->password(), QString("secretsecretsecret"));

    // making sure that the message is different if the database was not
    // modified by the merge operation.
    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    mergeCmd.execute({"merge", "-s", sourceFile.fileName(), sourceFile.fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine();
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Database was not modified by merge operation.\n"));

    // Quiet option
    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    mergeCmd.execute({"merge", "-q", "-s", sourceFile.fileName(), sourceFile.fileName()});
    m_stdoutFile->seek(pos);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));

    // Quiet option without the -s option
    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    Utils::Test::setNextPassword("a");
    mergeCmd.execute({"merge", "-q", sourceFile.fileName(), sourceFile.fileName()});
    m_stdoutFile->seek(pos);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
}

void TestCli::testMove()
{
    Move moveCmd;
    QVERIFY(!moveCmd.name.isEmpty());
    QVERIFY(moveCmd.getDescriptionLine().contains(moveCmd.name));

    qint64 pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    moveCmd.execute({"mv", m_dbFile->fileName(), "invalid_entry_path", "invalid_group_path"});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    m_stdoutFile->readLine(); // skip prompt line
    QCOMPARE(m_stdoutFile->readLine(), QByteArray(""));
    QCOMPARE(m_stderrFile->readLine(), QByteArray("Could not find entry with path invalid_entry_path.\n"));

    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    moveCmd.execute({"mv", m_dbFile->fileName(), "Sample Entry", "invalid_group_path"});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    m_stdoutFile->readLine(); // skip prompt line
    QCOMPARE(m_stdoutFile->readLine(), QByteArray(""));
    QCOMPARE(m_stderrFile->readLine(), QByteArray("Could not find group with path invalid_group_path.\n"));

    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    moveCmd.execute({"mv", m_dbFile->fileName(), "Sample Entry", "General/"});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    m_stdoutFile->readLine(); // skip prompt line
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Successfully moved entry Sample Entry to group General/.\n"));
    QCOMPARE(m_stderrFile->readLine(), QByteArray(""));

    auto db = readTestDatabase();
    auto* entry = db->rootGroup()->findEntryByPath("General/Sample Entry");
    QVERIFY(entry);

    // Test that not modified if the same group is destination.
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    moveCmd.execute({"mv", m_dbFile->fileName(), "General/Sample Entry", "General/"});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(posErr);
    m_stdoutFile->readLine(); // skip prompt line
    QCOMPARE(m_stdoutFile->readLine(), QByteArray(""));
    QCOMPARE(m_stderrFile->readLine(), QByteArray("Entry is already in group General/.\n"));

    // sanity check
    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("General/Sample Entry");
    QVERIFY(entry);
}

void TestCli::testRemove()
{
    Remove removeCmd;
    QVERIFY(!removeCmd.name.isEmpty());
    QVERIFY(removeCmd.getDescriptionLine().contains(removeCmd.name));

    Kdbx3Reader reader;
    Kdbx3Writer writer;

    // load test database and save a copy with disabled recycle bin
    auto db = readTestDatabase();
    QVERIFY(db);
    TemporaryFile fileCopy;
    fileCopy.open();
    db->metadata()->setRecycleBinEnabled(false);
    writer.writeDatabase(&fileCopy, db.data());
    fileCopy.close();

    qint64 pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();

    // delete entry and verify
    Utils::Test::setNextPassword("a");
    removeCmd.execute({"rm", m_dbFile->fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully recycled entry Sample Entry.\n"));
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));

    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));
    QFile readBack(m_dbFile->fileName());
    readBack.open(QIODevice::ReadOnly);
    auto readBackDb = QSharedPointer<Database>::create();
    reader.readDatabase(&readBack, key, readBackDb.data());
    readBack.close();
    QVERIFY(readBackDb);
    QVERIFY(!readBackDb->rootGroup()->findEntryByPath("/Sample Entry"));
    QVERIFY(readBackDb->rootGroup()->findEntryByPath(QString("/%1/Sample Entry").arg(Group::tr("Recycle Bin"))));

    pos = m_stdoutFile->pos();
    pos = m_stdoutFile->pos();

    // try again, this time without recycle bin
    Utils::Test::setNextPassword("a");
    removeCmd.execute({"rm", fileCopy.fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully deleted entry Sample Entry.\n"));

    readBack.setFileName(fileCopy.fileName());
    readBack.open(QIODevice::ReadOnly);
    readBackDb = QSharedPointer<Database>::create();
    reader.readDatabase(&readBack, key, readBackDb.data());
    readBack.close();
    QVERIFY(readBackDb);
    QVERIFY(!readBackDb->rootGroup()->findEntryByPath("/Sample Entry"));
    QVERIFY(!readBackDb->rootGroup()->findEntryByPath(QString("/%1/Sample Entry").arg(Group::tr("Recycle Bin"))));

    // finally, try deleting a non-existent entry
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    removeCmd.execute({"rm", fileCopy.fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Entry /Sample Entry not found.\n"));

    // try deleting a directory, should fail
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    removeCmd.execute({"rm", fileCopy.fileName(), "/General"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Entry /General not found.\n"));
}

void TestCli::testRemoveGroup()
{
    RemoveGroup removeGroupCmd;
    QVERIFY(!removeGroupCmd.name.isEmpty());
    QVERIFY(removeGroupCmd.getDescriptionLine().contains(removeGroupCmd.name));

    Kdbx3Reader reader;
    Kdbx3Writer writer;

    // try deleting a directory, should recycle it first.
    qint64 pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    removeGroupCmd.execute({"rmdir", m_dbFile->fileName(), "/General"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully recycled group /General.\n"));

    auto db = readTestDatabase();
    auto* group = db->rootGroup()->findGroupByPath("General");
    QVERIFY(!group);

    // try deleting a directory again, should delete it permanently.
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    removeGroupCmd.execute({"rmdir", m_dbFile->fileName(), "Recycle Bin/General"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully deleted group Recycle Bin/General.\n"));
    QCOMPARE(m_stderrFile->readAll(), QByteArray(""));

    db = readTestDatabase();
    group = db->rootGroup()->findGroupByPath("Recycle Bin/General");
    QVERIFY(!group);

    // try deleting an invalid group, should fail.
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    removeGroupCmd.execute({"rmdir", m_dbFile->fileName(), "invalid"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Group invalid not found.\n"));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));

    // Should fail to remove the root group.
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    removeGroupCmd.execute({"rmdir", m_dbFile->fileName(), "/"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Cannot remove root group from database.\n"));
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
}

void TestCli::testRemoveQuiet()
{
    Remove removeCmd;
    QVERIFY(!removeCmd.name.isEmpty());
    QVERIFY(removeCmd.getDescriptionLine().contains(removeCmd.name));

    Kdbx3Reader reader;
    Kdbx3Writer writer;

    qint64 pos = m_stdoutFile->pos();

    // delete entry and verify
    Utils::Test::setNextPassword("a");
    removeCmd.execute({"rm", "-q", m_dbFile->fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));

    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("a"));
    QFile readBack(m_dbFile->fileName());
    readBack.open(QIODevice::ReadOnly);
    auto readBackDb = QSharedPointer<Database>::create();
    reader.readDatabase(&readBack, key, readBackDb.data());
    readBack.close();
    QVERIFY(readBackDb);
    QVERIFY(!readBackDb->rootGroup()->findEntryByPath("/Sample Entry"));
    QVERIFY(readBackDb->rootGroup()->findEntryByPath(QString("/%1/Sample Entry").arg(Group::tr("Recycle Bin"))));

    pos = m_stdoutFile->pos();

    // remove the entry completely
    Utils::Test::setNextPassword("a");
    removeCmd.execute({"rm", "-q", m_dbFile->fileName(), QString("/%1/Sample Entry").arg(Group::tr("Recycle Bin"))});
    m_stdoutFile->seek(pos);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));

    readBack.setFileName(m_dbFile->fileName());
    readBack.open(QIODevice::ReadOnly);
    readBackDb = QSharedPointer<Database>::create();
    reader.readDatabase(&readBack, key, readBackDb.data());
    readBack.close();
    QVERIFY(readBackDb);
    QVERIFY(!readBackDb->rootGroup()->findEntryByPath("/Sample Entry"));
    QVERIFY(!readBackDb->rootGroup()->findEntryByPath(QString("/%1/Sample Entry").arg(Group::tr("Recycle Bin"))));
}

void TestCli::testShow()
{
    Show showCmd;
    QVERIFY(!showCmd.name.isEmpty());
    QVERIFY(showCmd.getDescriptionLine().contains(showCmd.name));

    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", m_dbFile->fileName(), "/Sample Entry"});
    m_stdoutFile->reset();
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("Title: Sample Entry\n"
                        "UserName: User Name\n"
                        "Password: PROTECTED\n"
                        "URL: http://www.somesite.com/\n"
                        "Notes: Notes\n"));

    qint64 pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", "-s", m_dbFile->fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("Title: Sample Entry\n"
                        "UserName: User Name\n"
                        "Password: Password\n"
                        "URL: http://www.somesite.com/\n"
                        "Notes: Notes\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", m_dbFile->fileName(), "-q", "/Sample Entry"});
    m_stdoutFile->seek(pos);
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("Title: Sample Entry\n"
                        "UserName: User Name\n"
                        "Password: PROTECTED\n"
                        "URL: http://www.somesite.com/\n"
                        "Notes: Notes\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", "-a", "Title", m_dbFile->fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Sample Entry\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", "-a", "Password", m_dbFile->fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Password\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", "-a", "Title", "-a", "URL", m_dbFile->fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("Sample Entry\n"
                        "http://www.somesite.com/\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", "-a", "DoesNotExist", m_dbFile->fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->reset();
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll(), QByteArray("ERROR: unknown attribute DoesNotExist.\n"));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", "-t", m_dbFile->fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QVERIFY(isTOTP(m_stdoutFile->readAll()));

    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", "-a", "Title", m_dbFile->fileName(), "--totp", "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Sample Entry\n"));
    QVERIFY(isTOTP(m_stdoutFile->readAll()));

    pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", m_dbFile2->fileName(), "--totp", "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Entry with path /Sample Entry has no TOTP set up.\n"));
}

void TestCli::testInvalidDbFiles()
{
    Show showCmd;
    QString nonExistentDbPath("/foo/bar/baz");
    QString directoryName("/");

    qint64 pos = m_stderrFile->pos();
    showCmd.execute({"show", nonExistentDbPath, "-q", "/Sample Entry"});
    m_stderrFile->seek(pos);
    QCOMPARE(QString(m_stderrFile->readAll()),
             QObject::tr("Failed to open database file %1: not found").arg(nonExistentDbPath) + "\n");

    pos = m_stderrFile->pos();
    showCmd.execute({"show", directoryName, "-q", "whatever"});
    m_stderrFile->seek(pos);
    QCOMPARE(QString(m_stderrFile->readAll()),
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
    pos = m_stderrFile->pos();
    showCmd.execute({"show", path, "some entry"});
    m_stderrFile->seek(pos);
    QCOMPARE(QString(m_stderrFile->readAll()),
             QObject::tr("Failed to open database file %1: not readable").arg(path) + "\n");
#endif // Q_OS_WIN
}

/**
 * Secret key for the YubiKey slot used by the unit test is
 * 1c e3 0f d7 8d 20 dc fa 40 b5 0c 18 77 9a fb 0f 02 28 8d b7
 * This secret should be configured at slot 2, and the slot
 * should be configured as passive.
 */
void TestCli::testYubiKeyOption()
{
    if (!YubiKey::instance()->init()) {
        QSKIP("Unable to connect to YubiKey");
    }

    QString errorMessage;
    bool isBlocking = YubiKey::instance()->checkSlotIsBlocking(2, errorMessage);
    if (isBlocking && errorMessage.isEmpty()) {
        QSKIP("Skipping YubiKey in press mode.");
    }

    QByteArray challenge("CLITest");
    QByteArray response;
    YubiKey::instance()->challenge(2, false, challenge, response);
    QByteArray expected("\xA2\x3B\x94\x00\xBE\x47\x9A\x30\xA9\xEB\x50\x9B\x85\x56\x5B\x6B\x30\x25\xB4\x8E", 20);
    QVERIFY2(response == expected, "YubiKey Slot 2 is not configured with correct secret key.");

    List listCmd;
    Add addCmd;

    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", "-y", "2", m_yubiKeyProtectedDbFile->fileName()});
    m_stdoutFile->reset();
    m_stderrFile->reset();
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("entry1\n"
                        "entry2\n"));

    // Should raise an error with no yubikey slot.
    qint64 pos = m_stdoutFile->pos();
    qint64 posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", m_yubiKeyProtectedDbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readLine(),
             QByteArray("Error while reading the database: Invalid credentials were provided, please try again.\n"));
    QCOMPARE(m_stderrFile->readLine(),
             QByteArray("If this reoccurs, then your database file may be corrupt. (HMAC mismatch)\n"));

    // Should raise an error if yubikey slot is not a string
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", "-y", "invalidslot", m_yubiKeyProtectedDbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll().split(':').at(0), QByteArray("Invalid YubiKey slot invalidslot\n"));

    // Should raise an error if yubikey slot is invalid.
    pos = m_stdoutFile->pos();
    posErr = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    listCmd.execute({"ls", "-y", "3", m_yubiKeyProtectedDbFile->fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->seek(posErr);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll().split(':').at(0), QByteArray("Invalid YubiKey slot 3\n"));
}

namespace
{

    void expectParseResult(const QString& input, const QStringList& expectedOutput)
    {
        QStringList result = Utils::splitCommandString(input);
        QCOMPARE(result.size(), expectedOutput.size());
        for (int i = 0; i < expectedOutput.size(); ++i) {
            QCOMPARE(result[i], expectedOutput[i]);
        }
    }

} // namespace

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

    expectParseResult(input, expectedOutput);
}

void TestCli::testOpen()
{
    Open o;

    Utils::Test::setNextPassword("a");
    o.execute({"open", m_dbFile->fileName()});
    m_stdoutFile->reset();
    QVERIFY(o.currentDatabase);

    List l;
    // Set a current database, simulating interactive mode.
    l.currentDatabase = o.currentDatabase;
    l.execute({"ls"});
    m_stdoutFile->reset();
    QByteArray expectedOutput("Sample Entry\n"
                              "General/\n"
                              "Windows/\n"
                              "Network/\n"
                              "Internet/\n"
                              "eMail/\n"
                              "Homebanking/\n");
    QByteArray actualOutput = m_stdoutFile->readAll();
    actualOutput.truncate(expectedOutput.length());
    QCOMPARE(actualOutput, expectedOutput);
}

void TestCli::testHelp()
{
    Help h;
    Commands::setupCommands(false);

    {
        h.execute({"help"});
        m_stderrFile->reset();
        QString output(m_stderrFile->readAll());
        QVERIFY(output.contains(QObject::tr("Available commands")));
    }

    {
        List l;
        h.execute({"help", "ls"});
        m_stderrFile->reset();
        QString output(m_stderrFile->readAll());
        QVERIFY(output.contains(l.description));
    }
}
