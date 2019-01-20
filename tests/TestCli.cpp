/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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
#include "core/PasswordGenerator.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "format/Kdbx3Reader.h"
#include "format/Kdbx3Writer.h"
#include "format/Kdbx4Reader.h"
#include "format/Kdbx4Writer.h"
#include "format/KdbxXmlReader.h"
#include "format/KeePass2.h"

#include "cli/Add.h"
#include "cli/Clip.h"
#include "cli/Command.h"
#include "cli/Create.h"
#include "cli/Diceware.h"
#include "cli/Edit.h"
#include "cli/Estimate.h"
#include "cli/Extract.h"
#include "cli/Generate.h"
#include "cli/List.h"
#include "cli/Locate.h"
#include "cli/Merge.h"
#include "cli/Remove.h"
#include "cli/Show.h"
#include "cli/Utils.h"

#include <QClipboard>
#include <QFile>
#include <QFuture>
#include <QSet>
#include <QtConcurrent>

#include <cstdio>

QTEST_MAIN(TestCli)

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

    // Load the NewDatabase.kdbx file into temporary storage
    QFile sourceDbFile2(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase2.kdbx"));
    QVERIFY(sourceDbFile2.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFile2, m_dbData2));
    sourceDbFile2.close();
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

    m_stdinFile.reset(new TemporaryFile());
    m_stdinFile->open();
    m_stdinHandle = fdopen(m_stdinFile->handle(), "r+");
    Utils::STDIN = m_stdinHandle;

    m_stdoutFile.reset(new TemporaryFile());
    m_stdoutFile->open();
    m_stdoutHandle = fdopen(m_stdoutFile->handle(), "r+");
    Utils::STDOUT = m_stdoutHandle;

    m_stderrFile.reset(new TemporaryFile());
    m_stderrFile->open();
    m_stderrHandle = fdopen(m_stderrFile->handle(), "r+");
    Utils::STDERR = m_stderrHandle;
}

void TestCli::cleanup()
{
    m_dbFile.reset();

    m_dbFile2.reset();

    m_stdinFile.reset();
    m_stdinHandle = stdin;
    Utils::STDIN = stdin;

    m_stdoutFile.reset();
    Utils::STDOUT = stdout;
    m_stdoutHandle = stdout;

    m_stderrFile.reset();
    m_stderrHandle = stderr;
    Utils::STDERR = stderr;
}

void TestCli::cleanupTestCase()
{
}

QSharedPointer<Database> TestCli::readTestDatabase() const
{
    Utils::Test::setNextPassword("a");
    auto db = QSharedPointer<Database>(Utils::unlockDatabase(m_dbFile->fileName(), "", m_stdoutHandle));
    m_stdoutFile->seek(ftell(m_stdoutHandle)); // re-synchronize handles
    return db;
}

void TestCli::testCommand()
{
    QCOMPARE(Command::getCommands().size(), 13);
    QVERIFY(Command::getCommand("add"));
    QVERIFY(Command::getCommand("clip"));
    QVERIFY(Command::getCommand("create"));
    QVERIFY(Command::getCommand("diceware"));
    QVERIFY(Command::getCommand("edit"));
    QVERIFY(Command::getCommand("estimate"));
    QVERIFY(Command::getCommand("extract"));
    QVERIFY(Command::getCommand("generate"));
    QVERIFY(Command::getCommand("locate"));
    QVERIFY(Command::getCommand("ls"));
    QVERIFY(Command::getCommand("merge"));
    QVERIFY(Command::getCommand("rm"));
    QVERIFY(Command::getCommand("show"));
    QVERIFY(!Command::getCommand("doesnotexist"));
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
                    "-l",
                    "20",
                    m_dbFile->fileName(),
                    "/newuser-entry"});
    m_stderrFile->reset();
    m_stdoutFile->reset();
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully added entry newuser-entry.\n"));

    auto db = readTestDatabase();
    auto* entry = db->rootGroup()->findEntryByPath("/newuser-entry");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://example.com/"));
    QCOMPARE(entry->password().size(), 20);

    // Quiet option
    qint64 pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    addCmd.execute({"add", "-q", "-u", "newuser", "-g", "-l", "20", m_dbFile->fileName(), "/newentry-quiet"});
    m_stdoutFile->seek(pos);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/newentry-quiet");
    QVERIFY(entry);

    Utils::Test::setNextPassword("a");
    Utils::Test::setNextPassword("newpassword");
    addCmd.execute({"add",
                    "-u",
                    "newuser2",
                    "--url",
                    "https://example.net/",
                    "-g",
                    "-l",
                    "20",
                    "-p",
                    m_dbFile->fileName(),
                    "/newuser-entry2"});

    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/newuser-entry2");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser2"));
    QCOMPARE(entry->url(), QString("https://example.net/"));
    QCOMPARE(entry->password(), QString("newpassword"));
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
    QFuture<void> future = QtConcurrent::run(&clipCmd, &Clip::execute, QStringList{"clip", m_dbFile->fileName(), "/Sample Entry", "1"});
    // clang-format on

    QTRY_COMPARE_WITH_TIMEOUT(clipboard->text(), QString("Password"), 500);
    QTRY_COMPARE_WITH_TIMEOUT(clipboard->text(), QString(""), 1500);

    future.waitForFinished();

    // TOTP with timeout
    Utils::Test::setNextPassword("a");
    future = QtConcurrent::run(
        &clipCmd, &Clip::execute, QStringList{"clip", m_dbFile->fileName(), "/Sample Entry", "1", "-t"});

    QTRY_VERIFY_WITH_TIMEOUT(isTOTP(clipboard->text()), 500);
    QTRY_COMPARE_WITH_TIMEOUT(clipboard->text(), QString(""), 1500);

    future.waitForFinished();

    qint64 posErr = m_stderrFile->pos();
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

    QString databaseFilename = testDir->path() + "testCreate1.kdbx";
    // Password
    Utils::Test::setNextPassword("a");
    createCmd.execute({"create", databaseFilename});

    m_stderrFile->reset();
    m_stdoutFile->reset();

    QCOMPARE(m_stdoutFile->readLine(),
             QByteArray("Insert password to encrypt database (Press enter to leave blank): \n"));
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Successfully created new database.\n"));

    Utils::Test::setNextPassword("a");
    auto db = QSharedPointer<Database>(Utils::unlockDatabase(databaseFilename, "", Utils::DEVNULL));
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
    QString databaseFilename2 = testDir->path() + "testCreate2.kdbx";
    QString keyfilePath = testDir->path() + "keyfile.txt";
    pos = m_stdoutFile->pos();
    errPos = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    createCmd.execute({"create", databaseFilename2, "-k", keyfilePath});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(errPos);

    QCOMPARE(m_stdoutFile->readLine(),
             QByteArray("Insert password to encrypt database (Press enter to leave blank): \n"));
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Successfully created new database.\n"));

    Utils::Test::setNextPassword("a");
    auto db2 = QSharedPointer<Database>(Utils::unlockDatabase(databaseFilename2, keyfilePath, Utils::DEVNULL));
    QVERIFY(db2);

    // Testing with existing keyfile
    QString databaseFilename3 = testDir->path() + "testCreate3.kdbx";
    pos = m_stdoutFile->pos();
    errPos = m_stderrFile->pos();
    Utils::Test::setNextPassword("a");
    createCmd.execute({"create", databaseFilename3, "-k", keyfilePath});
    m_stdoutFile->seek(pos);
    m_stderrFile->seek(errPos);

    QCOMPARE(m_stdoutFile->readLine(),
             QByteArray("Insert password to encrypt database (Press enter to leave blank): \n"));
    QCOMPARE(m_stdoutFile->readLine(), QByteArray("Successfully created new database.\n"));

    Utils::Test::setNextPassword("a");
    auto db3 = QSharedPointer<Database>(Utils::unlockDatabase(databaseFilename3, keyfilePath, Utils::DEVNULL));
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
    Utils::Test::setNextPassword("a");
    editCmd.execute({"edit", m_dbFile->fileName(), "-q", "-t", "newtitle", "/Sample Entry"});
    m_stdoutFile->seek(pos);
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));

    Utils::Test::setNextPassword("a");
    editCmd.execute({"edit", "-g", m_dbFile->fileName(), "/newtitle"});
    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/newtitle");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://otherurl.example.com/"));
    QVERIFY(!entry->password().isEmpty());
    QVERIFY(entry->password() != QString("Password"));

    Utils::Test::setNextPassword("a");
    editCmd.execute({"edit", "-g", "-l", "34", "-t", "yet another title", m_dbFile->fileName(), "/newtitle"});
    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/yet another title");
    QVERIFY(entry);
    QCOMPARE(entry->username(), QString("newuser"));
    QCOMPARE(entry->url(), QString("https://otherurl.example.com/"));
    QVERIFY(entry->password() != QString("Password"));
    QCOMPARE(entry->password().size(), 34);

    Utils::Test::setNextPassword("a");
    Utils::Test::setNextPassword("newpassword");
    editCmd.execute({"edit", "-p", m_dbFile->fileName(), "/yet another title"});
    db = readTestDatabase();
    entry = db->rootGroup()->findEntryByPath("/yet another title");
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

void TestCli::testExtract()
{
    Extract extractCmd;
    QVERIFY(!extractCmd.name.isEmpty());
    QVERIFY(extractCmd.getDescriptionLine().contains(extractCmd.name));

    Utils::Test::setNextPassword("a");
    extractCmd.execute({"extract", m_dbFile->fileName()});

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
    Utils::Test::setNextPassword("a");
    extractCmd.execute({"extract", "-q", m_dbFile->fileName()});
    m_stdoutFile->seek(pos);
    reader.readDatabase(m_stdoutFile.data(), dbQuiet.data());
    QVERIFY(!reader.hasError());
    QVERIFY(db.data());
}

void TestCli::testGenerate_data()
{
    QTest::addColumn<QStringList>("parameters");
    QTest::addColumn<QString>("pattern");

    QTest::newRow("default") << QStringList{"generate"} << "^[^\r\n]+$";
    QTest::newRow("length") << QStringList{"generate", "-L", "13"} << "^.{13}$";
    QTest::newRow("lowercase") << QStringList{"generate", "-L", "14", "-l"} << "^[a-z]{14}$";
    QTest::newRow("uppercase") << QStringList{"generate", "-L", "15", "-u"} << "^[A-Z]{15}$";
    QTest::newRow("numbers") << QStringList{"generate", "-L", "16", "-n"} << "^[0-9]{16}$";
    QTest::newRow("special") << QStringList{"generate", "-L", "200", "-s"}
                             << R"(^[\(\)\[\]\{\}\.\-*|\\,:;"'\/\_!+-<=>?#$%&^`@~]{200}$)";
    QTest::newRow("special (exclude)") << QStringList{"generate", "-L", "200", "-s", "-x", "+.?@&"}
                                       << R"(^[\(\)\[\]\{\}\.\-*|\\,:;"'\/\_!-<=>#$%^`~]{200}$)";
    QTest::newRow("extended") << QStringList{"generate", "-L", "50", "-e"}
                              << R"(^[^a-zA-Z0-9\(\)\[\]\{\}\.\-\*\|\\,:;"'\/\_!+-<=>?#$%&^`@~]{50}$)";
    QTest::newRow("numbers + lowercase + uppercase")
        << QStringList{"generate", "-L", "16", "-n", "-u", "-l"} << "^[0-9a-zA-Z]{16}$";
    QTest::newRow("numbers + lowercase + uppercase (exclude)")
        << QStringList{"generate", "-L", "500", "-n", "-u", "-l", "-x", "abcdefg0123@"} << "^[^abcdefg0123@]{500}$";
    QTest::newRow("numbers + lowercase + uppercase (exclude similar)")
        << QStringList{"generate", "-L", "200", "-n", "-u", "-l", "--exclude-similar"} << "^[^l1IO0]{200}$";
    QTest::newRow("uppercase + lowercase (every)")
        << QStringList{"generate", "-L", "2", "-u", "-l", "--every-group"} << "^[a-z][A-Z]|[A-Z][a-z]$";
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
                        "  [empty]\n"));

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
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("/Sample Entry\n/General/New Entry\n"));
}

void TestCli::testMerge()
{
    Merge mergeCmd;
    QVERIFY(!mergeCmd.name.isEmpty());
    QVERIFY(mergeCmd.getDescriptionLine().contains(mergeCmd.name));

    Kdbx4Writer writer;
    Kdbx4Reader reader;

    // load test database and save a copy
    auto db = readTestDatabase();
    QVERIFY(db);
    TemporaryFile targetFile1;
    targetFile1.open();
    writer.writeDatabase(&targetFile1, db.data());
    targetFile1.close();

    // save another copy with a different password
    TemporaryFile targetFile2;
    targetFile2.open();
    auto oldKey = db->key();
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create("b"));
    db->setKey(key);
    writer.writeDatabase(&targetFile2, db.data());
    targetFile2.close();
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
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully merged the database files.\n"));

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

    // try again with different passwords for both files
    pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("b");
    Utils::Test::setNextPassword("a");
    mergeCmd.execute({"merge", targetFile2.fileName(), sourceFile.fileName()});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine();
    m_stdoutFile->readLine();
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully merged the database files.\n"));

    readBack.setFileName(targetFile2.fileName());
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

    // delete entry and verify
    Utils::Test::setNextPassword("a");
    removeCmd.execute({"rm", m_dbFile->fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    QCOMPARE(m_stdoutFile->readAll(), QByteArray("Successfully recycled entry Sample Entry.\n"));

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

    pos = m_stdoutFile->pos();

    // finally, try deleting a non-existent entry
    Utils::Test::setNextPassword("a");
    removeCmd.execute({"rm", fileCopy.fileName(), "/Sample Entry"});
    m_stdoutFile->seek(pos);
    m_stdoutFile->readLine(); // skip password prompt
    m_stderrFile->reset();
    QCOMPARE(m_stdoutFile->readAll(), QByteArray(""));
    QCOMPARE(m_stderrFile->readAll(), QByteArray("Entry /Sample Entry not found.\n"));
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
                        "Password: Password\n"
                        "URL: http://www.somesite.com/\n"
                        "Notes: Notes\n"));

    qint64 pos = m_stdoutFile->pos();
    Utils::Test::setNextPassword("a");
    showCmd.execute({"show", m_dbFile->fileName(), "-q", "/Sample Entry"});
    m_stdoutFile->seek(pos);
    QCOMPARE(m_stdoutFile->readAll(),
             QByteArray("Title: Sample Entry\n"
                        "UserName: User Name\n"
                        "Password: Password\n"
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
