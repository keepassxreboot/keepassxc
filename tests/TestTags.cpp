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

#include <QTest>

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

void TestTags::testRemoveTag()
{
    QScopedPointer<Database> db(new Database());
    QVERIFY(db);

    auto* entry = new Entry();
    db->rootGroup()->addEntry(entry);
    QCOMPARE(entry->historyItems().size(), 0);

    entry->beginUpdate();
    entry->setTags("tag");
    entry->endUpdate();
    QCOMPARE(entry->tagList(), QStringList({"tag"}));
    QCOMPARE(entry->historyItems().size(), 1);

    db->removeTag("tag");
    QCOMPARE(entry->tagList().size(), 0);
    QCOMPARE(entry->historyItems().size(), 2);
}
