/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "TestConfig.h"

#include <QList>
#include <QTest>

#include "config-keepassx-tests.h"
#include "core/Config.h"
#include "gui/DatabaseWidgetStateSync.h"
#include "util/TemporaryFile.h"

QTEST_GUILESS_MAIN(TestConfig)

const QString oldTrueConfigPath = QString(KEEPASSX_TEST_DATA_DIR).append("/OutdatedConfig.ini");

// upgrade config file with deprecated settings set to non-default values
void TestConfig::testUpgrade()
{
    TemporaryFile tempFile;

    QVERIFY(tempFile.copyFromFile(oldTrueConfigPath));
    Config::createConfigFromFile(tempFile.fileName());

    QVERIFY(!config()->get("security/HidePasswordPreviewPanel").toBool());
    QVERIFY(config()->get("GUI/HidePreviewPanel").toBool());
    QVERIFY(config()->get("security/IconDownloadFallback").toBool());
    QVERIFY(config()->get("TrackNonDataChanges").toBool());
    QVERIFY(!config()->get("security/passwordsrepeatvisible").toBool());
    QVERIFY(!config()->get("security/passwordshidden").toBool());
    QVERIFY(config()->get("security/passwordemptyplaceholder").toBool());

    QList<int> intList;
    intList << 144 << 338;
    QCOMPARE(variantToIntList(config()->get("GUI/PreviewSplitterState")), intList);

    tempFile.remove();
}

QList<int> TestConfig::variantToIntList(const QVariant& variant)
{
    const QVariantList list = variant.toList();
    QList<int> result;

    for (const QVariant& var : list) {
        bool ok;
        int size = var.toInt(&ok);
        if (ok) {
            result.append(size);
        } else {
            result.clear();
            break;
        }
    }

    return result;
}
