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
#include "TestGlobal.h"
#include "core/DatabaseIcons.h"
#include "core/Metadata.h"
#include "crypto/Crypto.h"

void TestGuiPixmaps::initTestCase()
{
    QVERIFY(Crypto::init());
}

void TestGuiPixmaps::testDatabaseIcons()
{
    // check if the cache works correctly
    auto pixmap = databaseIcons()->icon(0);
    auto pixmapCached = databaseIcons()->icon(0);
    QCOMPARE(pixmapCached.cacheKey(), pixmap.cacheKey());
}

void TestGuiPixmaps::testEntryIcons()
{
    QScopedPointer<Database> db(new Database());
    Entry* entry = new Entry();
    entry->setGroup(db->rootGroup());

    // Test setting standard icon
    entry->setIcon(10);
    auto pixmap = entry->iconPixmap();
    QCOMPARE(pixmap.cacheKey(), databaseIcons()->icon(10).cacheKey());

    // Test setting custom icon
    QUuid iconUuid = QUuid::createUuid();
    QImage icon(2, 1, QImage::Format_RGB32);
    icon.setPixel(0, 0, qRgb(0, 0, 0));
    icon.setPixel(1, 0, qRgb(0, 0, 50));
    db->metadata()->addCustomIcon(iconUuid, icon);

    entry->setIcon(iconUuid);
    pixmap = entry->iconPixmap();
    QCOMPARE(pixmap.cacheKey(), db->metadata()->customIconPixmap(iconUuid).cacheKey());
}

void TestGuiPixmaps::testGroupIcons()
{
    QScopedPointer<Database> db(new Database());
    Group* group = db->rootGroup();

    // Test setting standard icon
    group->setIcon(10);
    auto pixmap = group->iconPixmap();
    QCOMPARE(pixmap.cacheKey(), databaseIcons()->icon(10).cacheKey());

    // Test setting custom icon
    QUuid iconUuid = QUuid::createUuid();
    QImage icon(2, 1, QImage::Format_RGB32);
    icon.setPixel(0, 0, qRgb(0, 0, 0));
    icon.setPixel(1, 0, qRgb(0, 0, 50));
    db->metadata()->addCustomIcon(iconUuid, icon);

    group->setIcon(iconUuid);
    pixmap = group->iconPixmap();
    QCOMPARE(pixmap.cacheKey(), db->metadata()->customIconPixmap(iconUuid).cacheKey());
}

QTEST_MAIN(TestGuiPixmaps)
