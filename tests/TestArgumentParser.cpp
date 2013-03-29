/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#include "TestArgumentParser.h"

#include <QtTest/QTest>

#include "tests.h"
#include "core/ArgumentParser.h"

// TODO: test qWarning with own message handler?

void TestArgumentParser::testNoArguments()
{
    parse(QStringList());

    QVERIFY(argumentMap.isEmpty());
}

void TestArgumentParser::testUnknownArgument()
{
    parse(QStringList() << "--foo" << "bar");

    QVERIFY(argumentMap.isEmpty());
}

void TestArgumentParser::testFilename()
{
    parse(QStringList() << "--filename" << "foo");

    QCOMPARE(argumentMap.size(), 1);
    QCOMPARE(argumentMap.value("filename"), QString("foo"));
}

void TestArgumentParser::testMultipleArguments()
{
    parse(QStringList() << "--config" << "myconfig.ini" << "--filename" << "myfilename" << "--password" << "mypassword");

    QCOMPARE(argumentMap.size(), 3);
    QCOMPARE(argumentMap.value("config"), QString("myconfig.ini"));
    QCOMPARE(argumentMap.value("filename"), QString("myfilename"));
    QCOMPARE(argumentMap.value("password"), QString("mypassword"));
}

void TestArgumentParser::testFilenameWithoutOption()
{
    parse(QStringList() << "foo");

    QCOMPARE(argumentMap.size(), 1);
    QCOMPARE(argumentMap.value("filename"), QString("foo"));
}

void TestArgumentParser::parse(const QStringList& arguments)
{
    argumentMap = ArgumentParser::parseArguments(QStringList() << "keepassx" << arguments);
}

QTEST_GUILESS_MAIN(TestArgumentParser)
