/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2014 Florian Geyer <blueice@fobos.de>
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

#include "TestExporter.h"

#include <QTest>

#include "tests.h"
#include "core/ToDbExporter.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"

QTEST_GUILESS_MAIN(TestExporter)

void TestExporter::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestExporter::testToDbExporter()
{
    QImage iconImage(1, 1, QImage::Format_RGB32);
    iconImage.setPixel(0, 0, qRgb(1, 2, 3));
    Uuid iconUuid = Uuid::random();

    QImage iconUnusedImage(1, 1, QImage::Format_RGB32);
    iconUnusedImage.setPixel(0, 0, qRgb(1, 2, 3));
    Uuid iconUnusedUuid = Uuid::random();

    Database* dbOrg = new Database();
    Group* groupOrg = new Group();
    groupOrg->setParent(dbOrg->rootGroup());
    groupOrg->setName("GTEST");
    Entry* entryOrg = new Entry();
    entryOrg->setGroup(groupOrg);
    entryOrg->setTitle("ETEST");
    dbOrg->metadata()->addCustomIcon(iconUuid, iconImage);
    dbOrg->metadata()->addCustomIcon(iconUnusedUuid, iconUnusedImage);
    entryOrg->setIcon(iconUuid);
    entryOrg->beginUpdate();
    entryOrg->setIcon(Entry::DefaultIconNumber);
    entryOrg->endUpdate();

    Database* dbExp = ToDbExporter().exportGroup(groupOrg);

    QCOMPARE(dbExp->rootGroup()->children().size(), 1);
    Group* groupExp = dbExp->rootGroup()->children().first();
    QVERIFY(groupExp != groupOrg);
    QCOMPARE(groupExp->name(), groupOrg->name());
    QCOMPARE(groupExp->entries().size(), 1);

    Entry* entryExp = groupExp->entries().first();
    QCOMPARE(entryExp->title(), entryOrg->title());
    QCOMPARE(dbExp->metadata()->customIcons().size(), 1);
    QVERIFY(dbExp->metadata()->containsCustomIcon(iconUuid));
    QCOMPARE(entryExp->iconNumber(), entryOrg->iconNumber());

    QCOMPARE(entryExp->historyItems().size(), 1);
    QCOMPARE(entryExp->historyItems().first()->iconUuid(), iconUuid);

    delete dbOrg;
    delete dbExp;
}



