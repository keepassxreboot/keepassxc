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

#include <QTest>

#include "config-keepassx-tests.h"
#include "util/TemporaryFile.h"

QTEST_GUILESS_MAIN(TestConfig)

const QString oldTrueConfigPath = QString(KEEPASSX_TEST_DATA_DIR).append("/OutdatedConfig.ini");

// upgrade config file with deprecated settings (all of which are set to non-default values)
void TestConfig::testUpgrade()
{
    TemporaryFile tempFile;

    QVERIFY(tempFile.copyFromFile(oldTrueConfigPath));
    Config::createConfigFromFile(tempFile.fileName());

    // value of new setting should be opposite the value of deprecated setting
    QVERIFY(!config()->get(Config::Security_PasswordsRepeatVisible).toBool());
    QVERIFY(!config()->get(Config::Security_PasswordsHidden).toBool());
    QVERIFY(config()->get(Config::Security_PasswordEmptyPlaceholder).toBool());

    tempFile.remove();
}
