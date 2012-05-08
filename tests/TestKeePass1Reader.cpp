/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "TestKeePass1Reader.h"

#include <QtTest/QTest>

#include "config-keepassx-tests.h"
#include "tests.h"
#include "core/Database.h"
#include "crypto/Crypto.h"
#include "format/KeePass1Reader.h"

void TestKeePass1Reader::initTestCase()
{
    Crypto::init();
}

void TestKeePass1Reader::testBasic()
{
    QString filename = QString(KEEPASSX_TEST_DATA_DIR).append("/basic.kdb");

    KeePass1Reader reader;
    Database* db = reader.readDatabase(filename, "masterpw", QByteArray());
    QVERIFY(db);
    QVERIFY(!reader.hasError());

    delete db;
}

KEEPASSX_QTEST_CORE_MAIN(TestKeePass1Reader)
