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

#include <QtTest/QTest>

#include "tests.h"
#include "core/Database.h"
#include "core/Group.h"
#include "crypto/Crypto.h"
#include "format/KeePass2XmlReader.h"
#include "config-keepassx-tests.h"

void TestDeletedObjects::initTestCase()
{
    Crypto::init();
}

void TestDeletedObjects::createAndDelete(Database* db, int delObjectsSize)
{
    QCOMPARE(db->deletedObjects().size(), delObjectsSize);
    Group* root = db->rootGroup();
    int rootChildrenCount = root->children().size();

    Group* g = new Group();
    g->setParent(root);
    Uuid gUuid = Uuid::random();
    g->setUuid(gUuid);
    delete g;
    QCOMPARE(db->deletedObjects().size(), ++delObjectsSize);
    QCOMPARE(db->deletedObjects().at(delObjectsSize-1).uuid, gUuid);
    QCOMPARE(rootChildrenCount, root->children().size());

    Group* g1 = new Group();
    g1->setParent(root);
    Uuid g1Uuid = Uuid::random();
    g1->setUuid(g1Uuid);
    Entry* e1 = new Entry();
    e1->setGroup(g1);
    Uuid e1Uuid = Uuid::random();
    e1->setUuid(e1Uuid);
    Group* g2 = new Group();
    g2->setParent(g1);
    Uuid g2Uuid = Uuid::random();
    g2->setUuid(g2Uuid);
    Entry* e2 = new Entry();
    e2->setGroup(g2);
    Uuid e2Uuid = Uuid::random();
    e2->setUuid(e2Uuid);

    delete g1;
    delObjectsSize += 4;

    QCOMPARE(db->deletedObjects().size(), delObjectsSize);
    QCOMPARE(db->deletedObjects().at(delObjectsSize-4).uuid, e1Uuid);
    QCOMPARE(db->deletedObjects().at(delObjectsSize-3).uuid, e2Uuid);
    QCOMPARE(db->deletedObjects().at(delObjectsSize-2).uuid, g2Uuid);
    QCOMPARE(db->deletedObjects().at(delObjectsSize-1).uuid, g1Uuid);
    QCOMPARE(rootChildrenCount, root->children().size());

    Entry* e3 = new Entry();
    e3->setGroup(root);
    Uuid e3Uuid = Uuid::random();
    e3->setUuid(e3Uuid);

    delete e3;

    QCOMPARE(db->deletedObjects().size(), ++delObjectsSize);
    QCOMPARE(db->deletedObjects().at(delObjectsSize-1).uuid, e3Uuid);
    QCOMPARE(rootChildrenCount, root->children().size());
}

void TestDeletedObjects::testDeletedObjectsFromFile()
{
    KeePass2XmlReader* reader = new KeePass2XmlReader();
    QString xmlFile = QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabase.xml");
    Database* db = reader->readDatabase(xmlFile);

    createAndDelete(db, 2);
}

void TestDeletedObjects::testDeletedObjectsFromNewDb()
{
    Database* db = new Database();

    createAndDelete(db, 0);
}

KEEPASSX_QTEST_CORE_MAIN(TestDeletedObjects)
