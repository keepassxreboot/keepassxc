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

#include "TestQrTotpSetupDialog.h"

#include <QComboBox>
#include <QLineEdit>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTabWidget>
#include <QTest>

#include "core/Entry.h"
#include "core/Totp.h"
#include "gui/TotpSetupDialog.h"
#include "qrdecoder/QrTotpSetupDialog.h"
#include "qrdecoder/QrTotpWidget.h"


QTEST_MAIN(TestQrTotpSetupDialog)

void TestQrTotpSetupDialog::testInitialTabs()
{
    Entry entry;
    QrDecoder::QrTotpSetupDialog dialog(nullptr, &entry);

    auto* tabWidget = dialog.findChild<QTabWidget*>();
    QVERIFY(tabWidget);

    QCOMPARE(tabWidget->count(), 2);
    QCOMPARE(tabWidget->tabText(0), QStringLiteral("Manual"));
    QCOMPARE(tabWidget->tabText(1), QStringLiteral("QR Code"));

    QCOMPARE(tabWidget->currentIndex(), 0);
}

void TestQrTotpSetupDialog::testSettingsReady()
{
    Entry entry;
    QrDecoder::QrTotpSetupDialog dialog(nullptr, &entry);

    auto* tabWidget = dialog.findChild<QTabWidget*>();
    QVERIFY(tabWidget);

    auto* qrWidget = dialog.findChild<QrDecoder::QrTotpWidget*>();
    QVERIFY(qrWidget);

    auto* totpDialog = dialog.findChild<TotpSetupDialog*>();
    QVERIFY(totpDialog);

    auto* seedEdit = totpDialog->findChild<QLineEdit*>("seedEdit");
    QVERIFY(seedEdit);

    auto* algorithmComboBox = totpDialog->findChild<QComboBox*>("algorithmComboBox");
    QVERIFY(algorithmComboBox);

    auto* stepSpinBox = totpDialog->findChild<QSpinBox*>("stepSpinBox");
    QVERIFY(stepSpinBox);

    auto* digitsSpinBox = totpDialog->findChild<QSpinBox*>("digitsSpinBox");
    QVERIFY(digitsSpinBox);

    Totp::Settings settings;
    settings.key = QStringLiteral("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ");
    settings.step = 60;
    settings.digits = 8;

    // Keep the algorithm from the default Settings object. We only need
    // to verify that setSettings() copies it to the combo box.
    const auto algorithm = settings.algorithm;

    const auto expectedAlgorithmIndex = algorithmComboBox->findData(algorithm);
    QVERIFY(expectedAlgorithmIndex >= 0);

    // The QR widget normally emits this signal after decoding and parsing
    // an otpauth URI. Here we trigger the same signal directly so this test
    // focuses only on QrTotpSetupDialog.
    emit qrWidget->settingsReady(QSharedPointer<Totp::Settings>::create(settings));

    QCOMPARE(tabWidget->currentWidget(), totpDialog);

    QCOMPARE(seedEdit->text(), settings.key);
    QCOMPARE(algorithmComboBox->currentIndex(), expectedAlgorithmIndex);
    QCOMPARE(stepSpinBox->value(), settings.step);
    QCOMPARE(digitsSpinBox->value(), settings.digits);
}

void TestQrTotpSetupDialog::testTotpUpdated()
{
    Entry entry;
    QrDecoder::QrTotpSetupDialog dialog(nullptr, &entry);

    auto* totpDialog = dialog.findChild<TotpSetupDialog*>();
    QVERIFY(totpDialog);

    QSignalSpy spy(&dialog, &QrDecoder::QrTotpSetupDialog::totpUpdated);

    emit totpDialog->totpUpdated();

    QCOMPARE(spy.count(), 1);
}
