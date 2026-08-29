/*
 *  Copyright (C) 2026 Brais Couce
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

#include "TestTags.h"

#include "QTest"

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "crypto/Crypto.h"

QTEST_GUILESS_MAIN(TestTags)

void TestTags::initTestCase()
{
    QVERIFY(Crypto::init());
    QLocale::setDefault(QLocale::c());
}

void TestTags::testRenameTag()
{
    QScopedPointer<Database> db(new Database());
    QVERIFY(db);

    auto* entry1 = new Entry();
    db->rootGroup()->addEntry(entry1);
    entry1->setTags("tag1, tag 2");

    auto* entry2 = new Entry();
    db->rootGroup()->addEntry(entry2);
    entry2->setTags("TaG 2, tag3");

    QString error;
    QVERIFY(db->renameTag("tag 2", "tag2_ren", &error));
    QVERIFY(error.isEmpty());

    QCOMPARE(entry1->tagList(), QStringList({"tag1", "tag2_ren"}));
    QCOMPARE(entry2->tagList(), QStringList({"tag2_ren", "tag3"}));
}

void TestTags::renameEmptyTag()
{
    QScopedPointer<Database> db(new Database());
    QVERIFY(db);

    QString error;

    QVERIFY(!db->renameTag("tag1", "   ", &error));
    QCOMPARE(error, QObject::tr("The tag name cannot be empty."));
    error.clear();

    QVERIFY(!db->renameTag("   ", "tag1_ren", &error));
    QCOMPARE(error, QObject::tr("The tag name cannot be empty."));
    error.clear();
}

void TestTags::renameExistingTag()
{
    QScopedPointer<Database> db(new Database());
    QVERIFY(db);

    auto* entry = new Entry();
    db->rootGroup()->addEntry(entry);
    entry->setTags("tag1, tag2");

    QString error;
    QVERIFY(!db->renameTag("tag1", "tag2", &error));
    QCOMPARE(error, QObject::tr("The tag \"%1\" already exists.").arg("tag2"));
}

void TestTags::testRenameNotExistingTag()
{
    QScopedPointer<Database> db(new Database());
    QVERIFY(db);

    auto* entry = new Entry();
    db->rootGroup()->addEntry(entry);
    entry->setTags("tag1, tag2");

    QString error;
    QVERIFY(!db->renameTag("tag3", "tag3_ren", &error));
    QCOMPARE(error, QObject::tr("The tag \"%1\" was not found.").arg("tag3"));
}
