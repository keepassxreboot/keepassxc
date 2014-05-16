/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "TestQCommandLineParser.h"

#include <QtTest>

#include "tests.h"
#include "core/qcommandlineparser.h"

QTEST_GUILESS_MAIN(TestQCommandLineParser)

Q_DECLARE_METATYPE(char**)

static char *empty_argv[] = { 0 };
static int empty_argc = 1;

void TestQCommandLineParser::initTestCase()
{
    Q_ASSERT(!empty_argv[0]);
    empty_argv[0] = const_cast<char*>("TestQCommandLineParser");
}

Q_DECLARE_METATYPE(QCommandLineParser::SingleDashWordOptionMode)

void TestQCommandLineParser::parsingModes_data()
{
    QTest::addColumn<QCommandLineParser::SingleDashWordOptionMode>("parsingMode");

    QTest::newRow("collapsed") << QCommandLineParser::ParseAsCompactedShortOptions;
    QTest::newRow("implicitlylong") << QCommandLineParser::ParseAsLongOptions;
}

void TestQCommandLineParser::testInvalidOptions()
{
    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    QTest::ignoreMessage(QtWarningMsg, "QCommandLineOption: Option names cannot start with a '-'");
    parser.addOption(QCommandLineOption("-v", "Displays version information."));
}

void TestQCommandLineParser::testPositionalArguments()
{
    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    QVERIFY(parser.parse(QStringList() << "TestQCommandLineParser" << "file.txt"));
    QCOMPARE(parser.positionalArguments(), QStringList() << "file.txt");
}

void TestQCommandLineParser::testBooleanOption_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QStringList>("expectedOptionNames");
    QTest::addColumn<bool>("expectedIsSet");

    QTest::newRow("set") << (QStringList() << "TestQCommandLineParser" << "-b") << (QStringList() << "b") << true;
    QTest::newRow("unset") << (QStringList() << "TestQCommandLineParser") << QStringList() << false;
}

void TestQCommandLineParser::testBooleanOption()
{
    QFETCH(QStringList, args);
    QFETCH(QStringList, expectedOptionNames);
    QFETCH(bool, expectedIsSet);
    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    QVERIFY(parser.addOption(QCommandLineOption("b", "a boolean option")));
    QVERIFY(parser.parse(args));
    QCOMPARE(parser.optionNames(), expectedOptionNames);
    QCOMPARE(parser.isSet("b"), expectedIsSet);
    QCOMPARE(parser.values("b"), QStringList());
    QCOMPARE(parser.positionalArguments(), QStringList());
    // Should warn on typos
    QTest::ignoreMessage(QtWarningMsg, "QCommandLineParser: option not defined: \"c\"");
    QVERIFY(!parser.isSet("c"));
}

void TestQCommandLineParser::testMultipleNames_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QStringList>("expectedOptionNames");

    QTest::newRow("short") << (QStringList() << "TestQCommandLineParser" << "-v") << (QStringList() << "v");
    QTest::newRow("long") << (QStringList() << "TestQCommandLineParser" << "--version") << (QStringList() << "version");
    QTest::newRow("not_set") << (QStringList() << "TestQCommandLineParser") << QStringList();
}

void TestQCommandLineParser::testMultipleNames()
{
    QFETCH(QStringList, args);
    QFETCH(QStringList, expectedOptionNames);
    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineOption option(QStringList() << "v" << "version", "Show version information");
    QCOMPARE(option.names(), QStringList() << "v" << "version");
    QCommandLineParser parser;
    QVERIFY(parser.addOption(option));
    QVERIFY(parser.parse(args));
    QCOMPARE(parser.optionNames(), expectedOptionNames);
    const bool expectedIsSet = !expectedOptionNames.isEmpty();
    QCOMPARE(parser.isSet("v"), expectedIsSet);
    QCOMPARE(parser.isSet("version"), expectedIsSet);
}

void TestQCommandLineParser::testSingleValueOption_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QStringList>("defaults");
    QTest::addColumn<bool>("expectedIsSet");

    QTest::newRow("short") << (QStringList() << "tst" << "-s" << "oxygen") << QStringList() << true;
    QTest::newRow("long") << (QStringList() << "tst" << "--style" << "oxygen") << QStringList() << true;
    QTest::newRow("longequal") << (QStringList() << "tst" << "--style=oxygen") << QStringList() << true;
    QTest::newRow("default") << (QStringList() << "tst") << (QStringList() << "oxygen") << false;
}

void TestQCommandLineParser::testSingleValueOption()
{
    QFETCH(QStringList, args);
    QFETCH(QStringList, defaults);
    QFETCH(bool, expectedIsSet);
    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    QCommandLineOption option(QStringList() << "s" << "style", "style name", "styleName");
    option.setDefaultValues(defaults);
    QVERIFY(parser.addOption(option));
    for (int mode = 0; mode < 2; ++mode) {
        parser.setSingleDashWordOptionMode(QCommandLineParser::SingleDashWordOptionMode(mode));
        QVERIFY(parser.parse(args));
        QCOMPARE(parser.isSet("s"), expectedIsSet);
        QCOMPARE(parser.isSet("style"), expectedIsSet);
        QCOMPARE(parser.isSet(option), expectedIsSet);
        QCOMPARE(parser.value("s"), QString("oxygen"));
        QCOMPARE(parser.value("style"), QString("oxygen"));
        QCOMPARE(parser.values("s"), QStringList() << "oxygen");
        QCOMPARE(parser.values("style"), QStringList() << "oxygen");
        QCOMPARE(parser.values(option), QStringList() << "oxygen");
        QCOMPARE(parser.positionalArguments(), QStringList());
    }
    // Should warn on typos
    QTest::ignoreMessage(QtWarningMsg, "QCommandLineParser: option not defined: \"c\"");
    QVERIFY(parser.values("c").isEmpty());
}

void TestQCommandLineParser::testValueNotSet()
{
    QCoreApplication app(empty_argc, empty_argv);
    // Not set, no default value
    QCommandLineParser parser;
    QCommandLineOption option(QStringList() << "s" << "style", "style name");
    option.setValueName("styleName");
    QVERIFY(parser.addOption(option));
    QVERIFY(parser.parse(QStringList() << "tst"));
    QCOMPARE(parser.optionNames(), QStringList());
    QVERIFY(!parser.isSet("s"));
    QVERIFY(!parser.isSet("style"));
    QCOMPARE(parser.value("s"), QString());
    QCOMPARE(parser.value("style"), QString());
    QCOMPARE(parser.values("s"), QStringList());
    QCOMPARE(parser.values("style"), QStringList());
}

void TestQCommandLineParser::testMultipleValuesOption()
{
    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineOption option("param", "Pass parameter to the backend.");
    option.setValueName("key=value");
    QCommandLineParser parser;
    QVERIFY(parser.addOption(option));
    {
        QVERIFY(parser.parse(QStringList() << "tst" << "--param" << "key1=value1"));
        QVERIFY(parser.isSet("param"));
        QCOMPARE(parser.values("param"), QStringList() << "key1=value1");
        QCOMPARE(parser.value("param"), QString("key1=value1"));
    }
    {
        QVERIFY(parser.parse(QStringList() << "tst" << "--param" << "key1=value1" << "--param" << "key2=value2"));
        QVERIFY(parser.isSet("param"));
        QCOMPARE(parser.values("param"), QStringList() << "key1=value1" << "key2=value2");
        QCOMPARE(parser.value("param"), QString("key2=value2"));
    }

    QString expected =
        "Usage: TestQCommandLineParser [options]\n"
        "\n"
        "Options:\n"
        "  --param <key=value>  Pass parameter to the backend.\n";

    const QString exeName = QCoreApplication::instance()->arguments().first(); // e.g. debug\tst_qcommandlineparser.exe on Windows
    expected.replace("TestQCommandLineParser", exeName);
    QCOMPARE(parser.helpText(), expected);
}

void TestQCommandLineParser::testUnknownOptionErrorHandling_data()
{
    QTest::addColumn<QCommandLineParser::SingleDashWordOptionMode>("parsingMode");
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QStringList>("expectedUnknownOptionNames");
    QTest::addColumn<QString>("expectedErrorText");

    const QStringList args_hello = QStringList() << "TestQCommandLineParser" << "--hello";
    const QString error_hello("Unknown option 'hello'.");
    QTest::newRow("unknown_name_collapsed") << QCommandLineParser::ParseAsCompactedShortOptions << args_hello << QStringList("hello") << error_hello;
    QTest::newRow("unknown_name_long") << QCommandLineParser::ParseAsLongOptions << args_hello << QStringList("hello") << error_hello;

    const QStringList args_value = QStringList() << "TestQCommandLineParser" << "-b=1";
    QTest::newRow("bool_with_value_collapsed") << QCommandLineParser::ParseAsCompactedShortOptions << args_value << QStringList() << QString("Unexpected value after '-b'.");
    QTest::newRow("bool_with_value_long") << QCommandLineParser::ParseAsLongOptions << args_value << QStringList() << QString("Unexpected value after '-b'.");

    const QStringList args_dash_long = QStringList() << "TestQCommandLineParser" << "-bool";
    const QString error_bool("Unknown options: o, o, l.");
    QTest::newRow("unknown_name_long_collapsed") << QCommandLineParser::ParseAsCompactedShortOptions << args_dash_long << (QStringList() << "o" << "o" << "l") << error_bool;
}

void TestQCommandLineParser::testUnknownOptionErrorHandling()
{
    QFETCH(QCommandLineParser::SingleDashWordOptionMode, parsingMode);
    QFETCH(QStringList, args);
    QFETCH(QStringList, expectedUnknownOptionNames);
    QFETCH(QString, expectedErrorText);

    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(parsingMode);
    QVERIFY(parser.addOption(QCommandLineOption(QStringList() << "b" << "bool", "a boolean option")));
    QCOMPARE(parser.parse(args), expectedErrorText.isEmpty());
    QCOMPARE(parser.unknownOptionNames(), expectedUnknownOptionNames);
    QCOMPARE(parser.errorText(), expectedErrorText);
}

void TestQCommandLineParser::testDoubleDash_data()
{
    parsingModes_data();
}

void TestQCommandLineParser::testDoubleDash()
{
    QFETCH(QCommandLineParser::SingleDashWordOptionMode, parsingMode);

    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList() << "o" << "output", "Output file", "filename"));
    parser.setSingleDashWordOptionMode(parsingMode);
    QVERIFY(parser.parse(QStringList() << "TestQCommandLineParser" << "--output" << "foo"));
    QCOMPARE(parser.value("output"), QString("foo"));
    QCOMPARE(parser.positionalArguments(), QStringList());
    QCOMPARE(parser.unknownOptionNames(), QStringList());
    QVERIFY(parser.parse(QStringList() << "TestQCommandLineParser" << "--" << "--output" << "bar" << "-b" << "bleh"));
    QCOMPARE(parser.value("output"), QString());
    QCOMPARE(parser.positionalArguments(), QStringList() << "--output" << "bar" << "-b" << "bleh");
    QCOMPARE(parser.unknownOptionNames(), QStringList());
}

void TestQCommandLineParser::testDefaultValue()
{
    QCommandLineOption opt("name", "desc",
                           "valueName", "default");
    QCOMPARE(opt.defaultValues(), QStringList("default"));
    opt.setDefaultValue("");
    QCOMPARE(opt.defaultValues(), QStringList());
    opt.setDefaultValue("default");
    QCOMPARE(opt.defaultValues(), QStringList("default"));
}

void TestQCommandLineParser::testProcessNotCalled()
{
    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    QVERIFY(parser.addOption(QCommandLineOption("b", "a boolean option")));
    QTest::ignoreMessage(QtWarningMsg, "QCommandLineParser: call process() or parse() before isSet");
    QVERIFY(!parser.isSet("b"));
    QTest::ignoreMessage(QtWarningMsg, "QCommandLineParser: call process() or parse() before values");
    QCOMPARE(parser.values("b"), QStringList());
}

void TestQCommandLineParser::testEmptyArgsList()
{
    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    QTest::ignoreMessage(QtWarningMsg, "QCommandLineParser: argument list cannot be empty, it should contain at least the executable name");
    QVERIFY(!parser.parse(QStringList())); // invalid call, argv[0] is missing
}

void TestQCommandLineParser::testMissingOptionValue()
{
    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption("option", "An option", "value"));
    QVERIFY(!parser.parse(QStringList() << "argv0" << "--option")); // the user forgot to pass a value for --option
    QCOMPARE(parser.value("option"), QString());
    QCOMPARE(parser.errorText(), QString("Missing value after '--option'."));
}

void TestQCommandLineParser::testStdinArgument_data()
{
    parsingModes_data();
}

void TestQCommandLineParser::testStdinArgument()
{
    QFETCH(QCommandLineParser::SingleDashWordOptionMode, parsingMode);

    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(parsingMode);
    parser.addOption(QCommandLineOption(QStringList() << "i" << "input", "Input file.", "filename"));
    parser.addOption(QCommandLineOption("b", "Boolean option."));
    QVERIFY(parser.parse(QStringList() << "TestQCommandLineParser" << "--input" << "-"));
    QCOMPARE(parser.value("input"), QString("-"));
    QCOMPARE(parser.positionalArguments(), QStringList());
    QCOMPARE(parser.unknownOptionNames(), QStringList());

    QVERIFY(parser.parse(QStringList() << "TestQCommandLineParser" << "--input" << "-" << "-b" << "arg"));
    QCOMPARE(parser.value("input"), QString("-"));
    QVERIFY(parser.isSet("b"));
    QCOMPARE(parser.positionalArguments(), QStringList() << "arg");
    QCOMPARE(parser.unknownOptionNames(), QStringList());

    QVERIFY(parser.parse(QStringList() << "TestQCommandLineParser" << "-"));
    QCOMPARE(parser.value("input"), QString());
    QVERIFY(!parser.isSet("b"));
    QCOMPARE(parser.positionalArguments(), QStringList() << "-");
    QCOMPARE(parser.unknownOptionNames(), QStringList());
}

void TestQCommandLineParser::testSingleDashWordOptionModes_data()
{
    QTest::addColumn<QCommandLineParser::SingleDashWordOptionMode>("parsingMode");
    QTest::addColumn<QStringList>("commandLine");
    QTest::addColumn<QStringList>("expectedOptionNames");
    QTest::addColumn<QStringList>("expectedOptionValues");

    QTest::newRow("collapsed") << QCommandLineParser::ParseAsCompactedShortOptions << (QStringList() << "-abc" << "val")
                               << (QStringList() << "a" << "b" << "c") << (QStringList() << QString() << QString() << "val");
    QTest::newRow("collapsed_with_equalsign_value") << QCommandLineParser::ParseAsCompactedShortOptions << (QStringList() << "-abc=val")
                               << (QStringList() << "a" << "b" << "c") << (QStringList() << QString() << QString() << "val");
    QTest::newRow("collapsed_explicit_longoption") << QCommandLineParser::ParseAsCompactedShortOptions << QStringList("--nn")
                               << QStringList("nn") << QStringList();
    QTest::newRow("collapsed_longoption_value") << QCommandLineParser::ParseAsCompactedShortOptions << (QStringList() << "--abc" << "val")
                               << QStringList("abc") << QStringList("val");
    QTest::newRow("compiler")  << QCommandLineParser::ParseAsCompactedShortOptions << QStringList("-cab")
                               << QStringList("c") << QStringList("ab");
    QTest::newRow("compiler_with_space") << QCommandLineParser::ParseAsCompactedShortOptions << (QStringList() << "-c" << "val")
                               << QStringList("c") << QStringList("val");

    QTest::newRow("implicitlylong") << QCommandLineParser::ParseAsLongOptions << (QStringList() << "-abc" << "val")
                               << QStringList("abc") << QStringList("val");
    QTest::newRow("implicitlylong_equal") << QCommandLineParser::ParseAsLongOptions << (QStringList() << "-abc=val")
                               << QStringList("abc") << QStringList("val");
    QTest::newRow("implicitlylong_longoption") << QCommandLineParser::ParseAsLongOptions << (QStringList() << "--nn")
                               << QStringList("nn") << QStringList();
    QTest::newRow("implicitlylong_longoption_value") << QCommandLineParser::ParseAsLongOptions << (QStringList() << "--abc" << "val")
                               << QStringList("abc") << QStringList("val");
    QTest::newRow("implicitlylong_with_space") << QCommandLineParser::ParseAsCompactedShortOptions << (QStringList() << "-c" << "val")
                               << QStringList("c") << QStringList("val");
}

void TestQCommandLineParser::testSingleDashWordOptionModes()
{
    QFETCH(QCommandLineParser::SingleDashWordOptionMode, parsingMode);
    QFETCH(QStringList, commandLine);
    QFETCH(QStringList, expectedOptionNames);
    QFETCH(QStringList, expectedOptionValues);

    commandLine.prepend("TestQCommandLineParser");

    QCoreApplication app(empty_argc, empty_argv);
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(parsingMode);
    parser.addOption(QCommandLineOption("a", "a option."));
    parser.addOption(QCommandLineOption("b", "b option."));
    parser.addOption(QCommandLineOption(QStringList() << "c" << "abc", "c option.", "value"));
    parser.addOption(QCommandLineOption("nn", "nn option."));
    QVERIFY(parser.parse(commandLine));
    QCOMPARE(parser.optionNames(), expectedOptionNames);
    for (int i = 0; i < expectedOptionValues.count(); ++i)
        QCOMPARE(parser.value(parser.optionNames().at(i)), expectedOptionValues.at(i));
    QCOMPARE(parser.unknownOptionNames(), QStringList());
}
