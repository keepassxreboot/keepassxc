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

#include <QSettings>
#include <QTest>

#include "config-keepassx-tests.h"
#include "util/TemporaryFile.h"

QTEST_GUILESS_MAIN(TestConfig)

const QString oldTrueConfigPath = QString(KEEPASSX_TEST_DATA_DIR).append("/OutdatedConfig.ini");

void TestConfig::initTestCase()
{
    QLocale::setDefault(QLocale::c());
}

// upgrade config file with deprecated settings (all of which are set to non-default values)
void TestConfig::testUpgrade()
{
    TemporaryFile tempFile;
    TemporaryFile localFile;
    QVERIFY(localFile.open());
    localFile.close();

    QVERIFY(tempFile.copyFromFile(oldTrueConfigPath));
    Config::createConfigFromFile(tempFile.fileName(), localFile.fileName());

    // value of new setting should be opposite the value of deprecated setting
    QVERIFY(!config()->get(Config::Security_PasswordsHidden).toBool());
    QVERIFY(config()->get(Config::Security_PasswordEmptyPlaceholder).toBool());

    tempFile.remove();
}

void TestConfig::testDeprecatedSettingMigration_data()
{
    QTest::addColumn<bool>("localSource");
    QTest::addColumn<QString>("targetLocation");
    QTest::addColumn<QByteArray>("targetValue");

    QTest::newRow("roaming-source-missing-target") << false << QString() << QByteArray();
    QTest::newRow("local-source-missing-target") << true << QString() << QByteArray();
    QTest::newRow("roaming-source-roaming-target") << false << QString("roaming") << QByteArray("existing");
    QTest::newRow("roaming-source-local-target") << false << QString("local") << QByteArray("existing");
    QTest::newRow("local-source-roaming-target") << true << QString("roaming") << QByteArray("existing");
    QTest::newRow("local-source-local-target") << true << QString("local") << QByteArray("existing");
    QTest::newRow("existing-empty-target") << false << QString("local") << QByteArray();
}

void TestConfig::testDeprecatedSettingMigration()
{
    QFETCH(bool, localSource);
    QFETCH(QString, targetLocation);
    QFETCH(QByteArray, targetValue);

    TemporaryFile roamingFile;
    TemporaryFile localFile;
    QVERIFY(roamingFile.open());
    QVERIFY(localFile.open());
    roamingFile.close();
    localFile.close();

    const auto oldName = QStringLiteral("GUI/DetailSplitterState");
    const auto newName = QStringLiteral("GUI/PreviewSplitterState");
    const auto oldValue = QByteArray("deprecated");
    {
        QSettings roaming(roamingFile.fileName(), QSettings::IniFormat);
        QSettings local(localFile.fileName(), QSettings::IniFormat);
        roaming.setValue("ConfigVersion", 2);
        (localSource ? local : roaming).setValue(oldName, oldValue);
        if (!targetLocation.isEmpty()) {
            (targetLocation == "local" ? local : roaming).setValue(newName, targetValue);
        }
        roaming.sync();
        local.sync();
    }

    Config::createConfigFromFile(roamingFile.fileName(), localFile.fileName());
    QCOMPARE(config()->get(Config::GUI_PreviewSplitterState).toByteArray(),
             targetLocation.isEmpty() ? oldValue : targetValue);

    QSettings roaming(roamingFile.fileName(), QSettings::IniFormat);
    QSettings local(localFile.fileName(), QSettings::IniFormat);
    QVERIFY(!roaming.contains(oldName));
    QVERIFY(!local.contains(oldName));
}

void TestConfig::testLocalSettingMigration_data()
{
    QTest::addColumn<bool>("targetExists");
    QTest::addColumn<QByteArray>("targetValue");

    QTest::newRow("missing-local-target") << false << QByteArray();
    QTest::newRow("existing-local-target") << true << QByteArray("local");
    QTest::newRow("existing-empty-local-target") << true << QByteArray();
}

void TestConfig::testLocalSettingMigration()
{
    QFETCH(bool, targetExists);
    QFETCH(QByteArray, targetValue);

    TemporaryFile roamingFile;
    TemporaryFile localFile;
    QVERIFY(roamingFile.open());
    QVERIFY(localFile.open());
    roamingFile.close();
    localFile.close();

    const auto name = QStringLiteral("GUI/PreviewSplitterState");
    const auto roamingValue = QByteArray("roaming");
    {
        QSettings roaming(roamingFile.fileName(), QSettings::IniFormat);
        QSettings local(localFile.fileName(), QSettings::IniFormat);
        roaming.setValue("ConfigVersion", 2);
        roaming.setValue(name, roamingValue);
        if (targetExists) {
            local.setValue(name, targetValue);
        }
        roaming.sync();
        local.sync();
    }

    Config::createConfigFromFile(roamingFile.fileName(), localFile.fileName());
    QCOMPARE(config()->get(Config::GUI_PreviewSplitterState).toByteArray(), targetExists ? targetValue : roamingValue);

    QSettings roaming(roamingFile.fileName(), QSettings::IniFormat);
    QSettings local(localFile.fileName(), QSettings::IniFormat);
    QVERIFY(!roaming.contains(name));
    QVERIFY(local.contains(name));
    QCOMPARE(local.value(name).toByteArray(), targetExists ? targetValue : roamingValue);
}

void TestConfig::testSaveMethodMigration_data()
{
    QTest::addColumn<int>("version");
    QTest::addColumn<bool>("atomic");
    QTest::addColumn<bool>("directWrite");
    QTest::addColumn<bool>("expectedAtomic");

    QTest::newRow("old-temporary-file") << 2 << false << false << true;
    QTest::newRow("old-atomic") << 2 << true << false << true;
    QTest::newRow("old-direct-write") << 2 << false << true << false;
    QTest::newRow("current-temporary-file") << 3 << false << false << false;
}

void TestConfig::testSaveMethodMigration()
{
    QFETCH(int, version);
    QFETCH(bool, atomic);
    QFETCH(bool, directWrite);
    QFETCH(bool, expectedAtomic);

    TemporaryFile tempFile;
    TemporaryFile localFile;
    QVERIFY(tempFile.open());
    QVERIFY(localFile.open());
    tempFile.close();
    localFile.close();
    {
        QSettings settings(tempFile.fileName(), QSettings::IniFormat);
        settings.setValue("ConfigVersion", version);
        settings.setValue("UseAtomicSaves", atomic);
        settings.setValue("UseDirectWriteSaves", directWrite);
        settings.sync();
    }

    Config::createConfigFromFile(tempFile.fileName(), localFile.fileName());
    QCOMPARE(config()->get(Config::UseAtomicSaves).toBool(), expectedAtomic);
    QCOMPARE(config()->get(Config::UseDirectWriteSaves).toBool(), directWrite);
    QSettings settings(tempFile.fileName(), QSettings::IniFormat);
    QCOMPARE(settings.value("ConfigVersion").toInt(), 3);
}

void TestConfig::testURLDoubleClickMigration()
{
    // Test migration from OpenURLOnDoubleClick to URLDoubleClickAction
    TemporaryFile tempFile;
    TemporaryFile localFile;
    QVERIFY(tempFile.open());
    QVERIFY(localFile.open());
    localFile.close();

    // Create a config with old setting = true (open browser)
    QSettings oldConfig(tempFile.fileName(), QSettings::IniFormat);
    oldConfig.setValue("OpenURLOnDoubleClick", true);
    oldConfig.sync();
    tempFile.close();

    Config::createConfigFromFile(tempFile.fileName(), localFile.fileName());

    // Should migrate to URLDoubleClickAction = 0 (open browser)
    QCOMPARE(config()->get(Config::URLDoubleClickAction).toInt(), 0);

    tempFile.remove();

    // Test migration from OpenURLOnDoubleClick = false (edit entry)
    TemporaryFile tempFile2;
    TemporaryFile localFile2;
    QVERIFY(tempFile2.open());
    QVERIFY(localFile2.open());
    localFile2.close();

    QSettings oldConfig2(tempFile2.fileName(), QSettings::IniFormat);
    oldConfig2.setValue("OpenURLOnDoubleClick", false);
    oldConfig2.sync();
    tempFile2.close();

    Config::createConfigFromFile(tempFile2.fileName(), localFile2.fileName());

    // Should migrate to URLDoubleClickAction = 2 (edit entry)
    QCOMPARE(config()->get(Config::URLDoubleClickAction).toInt(), 2);

    tempFile2.remove();
}
