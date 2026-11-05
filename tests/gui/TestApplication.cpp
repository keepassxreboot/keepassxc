/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "TestApplication.h"

#include <QDir>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "core/Config.h"
#include "gui/Application.h"

int main(int argc, char* argv[])
{
    const auto testUsername = QStringLiteral("keepassxc-test-%1").arg(QCoreApplication::applicationPid()).toUtf8();
    qputenv("USER", testUsername);
    qputenv("USERNAME", testUsername);

    const auto temporaryPath = QDir::currentPath().toUtf8();
    qputenv("TMPDIR", temporaryPath);
    qputenv("TEMP", temporaryPath);
    qputenv("TMP", temporaryPath);

    QTemporaryDir temporaryDirectory;
    if (!temporaryDirectory.isValid()) {
        return EXIT_FAILURE;
    }

    const auto configPath = temporaryDirectory.filePath("keepassxc.ini").toUtf8();
    qputenv("KPXC_CONFIG", configPath);
    qputenv("KPXC_CONFIG_LOCAL", configPath);

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    Application app(argc, argv);

    TestApplication tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

void TestApplication::initTestCase()
{
    config()->set(Config::SingleInstance, true);
    kpxcApp->initializeSingleInstance();
    QVERIFY(!kpxcApp->isAlreadyRunning());
}

void TestApplication::testInformationalCommandsDoNotActivateExistingInstance_data()
{
    QTest::addColumn<QString>("argument");

    QTest::newRow("version") << QStringLiteral("--version");
    QTest::newRow("help") << QStringLiteral("--help");
    QTest::newRow("debug-info") << QStringLiteral("--debug-info");
}

void TestApplication::testInformationalCommandsDoNotActivateExistingInstance()
{
    QFETCH(QString, argument);

    QSignalSpy instanceStarted(kpxcApp, &Application::anotherInstanceStarted);
    QProcess process;
    process.start(QStringLiteral(KEEPASSXC_BINARY), {argument});
    QVERIFY2(process.waitForFinished(), qPrintable(process.errorString()));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), EXIT_SUCCESS);
    if (argument == "--version") {
        QVERIFY(process.readAllStandardOutput().startsWith("KeePassXC "));
    }

    QTest::qWait(250);
    QCOMPARE(instanceStarted.count(), 0);
}

void TestApplication::testSecondInstanceActivatesExistingInstance()
{
    QSignalSpy instanceStarted(kpxcApp, &Application::anotherInstanceStarted);
    QProcess process;
    process.start(QStringLiteral(KEEPASSXC_BINARY));
    QVERIFY2(process.waitForFinished(), qPrintable(process.errorString()));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), EXIT_SUCCESS);

    QTRY_COMPARE(instanceStarted.count(), 1);
}
