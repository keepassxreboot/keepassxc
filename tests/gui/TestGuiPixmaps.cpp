/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "TestGuiPixmaps.h"
#include "core/Metadata.h"

#include <QTest>

#include "core/Group.h"
#include "crypto/Crypto.h"
#include "gui/DatabaseIcons.h"
#include "gui/Icons.h"

void TestGuiPixmaps::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestGuiPixmaps::testDatabaseIcons()
{
    QVERIFY(!databaseIcons()->icon(0).isNull());
}

void TestGuiPixmaps::testEntryIcons()
{
    QScopedPointer<Database> db(new Database());
    auto entry = new Entry();
    entry->setGroup(db->rootGroup());

    // Test setting standard icon
    entry->setIcon(10);
    auto pixmap = Icons::entryIconPixmap(entry);
    QVERIFY(pixmap.toImage() == databaseIcons()->icon(10).toImage());

    // Test setting custom icon
    QUuid iconUuid = QUuid::createUuid();
    QImage icon(2, 1, QImage::Format_RGB32);
    icon.setPixel(0, 0, qRgb(0, 0, 0));
    icon.setPixel(1, 0, qRgb(0, 0, 50));
    db->metadata()->addCustomIcon(iconUuid, Icons::saveToBytes(icon));
    QCOMPARE(db->metadata()->customIconsOrder().count(), 1);

    entry->setIcon(iconUuid);
    // Confirm the icon is the same as that stored in the database
    QVERIFY(Icons::entryIconPixmap(entry).toImage() == Icons::customIconPixmap(db.data(), iconUuid).toImage());
}

void TestGuiPixmaps::testGroupIcons()
{
    QScopedPointer<Database> db(new Database());
    auto group = db->rootGroup();

    // Test setting standard icon
    group->setIcon(10);
    auto pixmap = Icons::groupIconPixmap(group);
    QVERIFY(pixmap.toImage() == databaseIcons()->icon(10).toImage());

    // Test setting custom icon
    QUuid iconUuid = QUuid::createUuid();
    QImage icon(2, 1, QImage::Format_RGB32);
    icon.setPixel(0, 0, qRgb(0, 0, 0));
    icon.setPixel(1, 0, qRgb(0, 0, 50));
    db->metadata()->addCustomIcon(iconUuid, Icons::saveToBytes(icon));

    group->setIcon(iconUuid);
    // Confirm the icon is the same as that stored in the database
    QVERIFY(Icons::groupIconPixmap(group).toImage() == Icons::customIconPixmap(db.data(), iconUuid).toImage());
}

QTEST_MAIN(TestGuiPixmaps)
