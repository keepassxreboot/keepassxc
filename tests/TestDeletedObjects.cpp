/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "TestDeletedObjects.h"

#include "config-keepassx-tests.h"
#include "core/Group.h"
#include "crypto/Crypto.h"
#include "format/KdbxXmlReader.h"
#include "format/KeePass2.h"
#include <QTest>

QTEST_GUILESS_MAIN(TestDeletedObjects)

void TestDeletedObjects::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestDeletedObjects::createAndDelete(QSharedPointer<Database> db, int delObjectsSize)
{
    QCOMPARE(db->deletedObjects().size(), delObjectsSize);
    Group* root = db->rootGroup();
    int rootChildrenCount = root->children().size();

    auto g = new Group();
    g->setParent(root);
    QUuid gUuid = QUuid::createUuid();
    g->setUuid(gUuid);
    delete g;
    QCOMPARE(db->deletedObjects().size(), ++delObjectsSize);
    QCOMPARE(db->deletedObjects().at(delObjectsSize - 1).uuid, gUuid);
    QCOMPARE(rootChildrenCount, root->children().size());

    auto g1 = new Group();
    g1->setParent(root);
    QUuid g1Uuid = QUuid::createUuid();
    g1->setUuid(g1Uuid);
    auto e1 = new Entry();
    e1->setGroup(g1);
    QUuid e1Uuid = QUuid::createUuid();
    e1->setUuid(e1Uuid);
    auto g2 = new Group();
    g2->setParent(g1);
    QUuid g2Uuid = QUuid::createUuid();
    g2->setUuid(g2Uuid);
    auto e2 = new Entry();
    e2->setGroup(g2);
    QUuid e2Uuid = QUuid::createUuid();
    e2->setUuid(e2Uuid);

    delete g1;
    delObjectsSize += 4;

    QCOMPARE(db->deletedObjects().size(), delObjectsSize);
    QCOMPARE(db->deletedObjects().at(delObjectsSize - 4).uuid, e1Uuid);
    QCOMPARE(db->deletedObjects().at(delObjectsSize - 3).uuid, e2Uuid);
    QCOMPARE(db->deletedObjects().at(delObjectsSize - 2).uuid, g2Uuid);
    QCOMPARE(db->deletedObjects().at(delObjectsSize - 1).uuid, g1Uuid);
    QCOMPARE(rootChildrenCount, root->children().size());

    auto e3 = new Entry();
    e3->setGroup(root);
    QUuid e3Uuid = QUuid::createUuid();
    e3->setUuid(e3Uuid);

    delete e3;

    QCOMPARE(db->deletedObjects().size(), ++delObjectsSize);
    QCOMPARE(db->deletedObjects().at(delObjectsSize - 1).uuid, e3Uuid);
    QCOMPARE(rootChildrenCount, root->children().size());
}

void TestDeletedObjects::testDeletedObjectsFromFile()
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_3_1);
    reader.setStrictMode(true);
    QString xmlFile = QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.xml");
    auto db = reader.readDatabase(xmlFile);

    createAndDelete(db, 2);
}

void TestDeletedObjects::testDeletedObjectsFromNewDb()
{
    auto db = QSharedPointer<Database>::create();
    createAndDelete(db, 0);
}

void TestDeletedObjects::testDatabaseChange()
{
    auto db = QSharedPointer<Database>::create();
    Group* root = db->rootGroup();
    int delObjectsSize = 0;
    auto db2 = QSharedPointer<Database>::create();
    Group* root2 = db2->rootGroup();
    int delObjectsSize2 = 0;

    auto* e = new Entry();
    e->setGroup(root);

    QCOMPARE(db->deletedObjects().size(), delObjectsSize);
    QCOMPARE(db2->deletedObjects().size(), delObjectsSize2);

    e->setGroup(root2);

    QCOMPARE(db->deletedObjects().size(), ++delObjectsSize);
    QCOMPARE(db2->deletedObjects().size(), delObjectsSize2);

    delete e;

    QCOMPARE(db->deletedObjects().size(), delObjectsSize);
    QCOMPARE(db2->deletedObjects().size(), ++delObjectsSize2);

    auto* g1 = new Group();
    g1->setParent(root);
    QUuid g1Uuid = QUuid::createUuid();
    g1->setUuid(g1Uuid);
    auto* e1 = new Entry();
    e1->setGroup(g1);
    QUuid e1Uuid = QUuid::createUuid();
    e1->setUuid(e1Uuid);
    g1->setParent(root2);

    delObjectsSize += 2;
    QCOMPARE(db->deletedObjects().size(), delObjectsSize);
    QCOMPARE(db2->deletedObjects().size(), delObjectsSize2);
    QCOMPARE(db->deletedObjects().at(delObjectsSize - 2).uuid, e1Uuid);
    QCOMPARE(db->deletedObjects().at(delObjectsSize - 1).uuid, g1Uuid);

    auto* group = new Group();
    auto* entry = new Entry();
    entry->setGroup(group);
    entry->setGroup(root);

    QCOMPARE(db->deletedObjects().size(), delObjectsSize);
    QCOMPARE(db2->deletedObjects().size(), delObjectsSize2);

    delete group;
}

void TestDeletedObjects::testCustomIconDeletion()
{
    Database db;
    QCOMPARE(db.deletedObjects().size(), 0);

    QUuid uuid = QUuid::createUuid();
    db.metadata()->addCustomIcon(uuid, QByteArray());
    db.metadata()->removeCustomIcon(uuid);
    QCOMPARE(db.deletedObjects().size(), 1);
    QCOMPARE(db.deletedObjects().at(0).uuid, uuid);
}
