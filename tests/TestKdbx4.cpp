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

#include "TestKdbx4.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"
#include "format/KeePass2.h"
#include "format/KdbxXmlReader.h"
#include "format/KdbxXmlWriter.h"
#include "config-keepassx-tests.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestKdbx4)

void TestKdbx4::initTestCase()
{
    QVERIFY(Crypto::init());

    KdbxXmlReader reader(KeePass2::FILE_VERSION_3);
    reader.setStrictMode(true);
    QString xmlFile = QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.xml");
    m_db.reset(reader.readDatabase(xmlFile));
    QVERIFY(m_db.data());
    QVERIFY(!reader.hasError());
}

Database* TestKdbx4::readDatabase(QString path, bool strictMode, bool& hasError, QString& errorString)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_3);
    reader.setStrictMode(strictMode);
    auto db = reader.readDatabase(path);
    hasError = reader.hasError();
    errorString = reader.errorString();
    return db;
}

Database* TestKdbx4::readDatabase(QBuffer* buf, bool strictMode, bool& hasError, QString& errorString)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_3);
    reader.setStrictMode(strictMode);
    auto db = reader.readDatabase(buf);
    hasError = reader.hasError();
    errorString = reader.errorString();
    return db;
}

void TestKdbx4::writeDatabase(QBuffer* buf, Database* db, bool& hasError, QString& errorString)
{
    KdbxXmlWriter writer(KeePass2::FILE_VERSION_3);
    writer.writeDatabase(buf, db);
    hasError = writer.hasError();
    errorString = writer.errorString();
}
