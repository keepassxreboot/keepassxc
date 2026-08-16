/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TestQrTotpWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

#include "core/Totp.h"
#include "qrdecoder/QrTotpWidget.h"

QTEST_MAIN(TestQrTotpWidget)

namespace
{
    const QString validUri = QStringLiteral("otpauth://totp/"
                                            "ACME%20Co:john@example.com"
                                            "?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"
                                            "&issuer=ACME%20Co"
                                            "&algorithm=SHA1"
                                            "&digits=6"
                                            "&period=30");

    QPushButton* findButton(QWidget* widget, const QString& text)
    {
        const auto buttons = widget->findChildren<QPushButton*>();

        for (auto* button : buttons) {
            if (button->text() == text) {
                return button;
            }
        }

        return nullptr;
    }
} // namespace

void TestQrTotpWidget::init()
{
    m_clipboardText = QApplication::clipboard()->text();
    qRegisterMetaType<QSharedPointer<Totp::Settings>>();
}

void TestQrTotpWidget::cleanup()
{
    QApplication::clipboard()->setText(m_clipboardText);
}

void TestQrTotpWidget::testInitialState()
{
    QrDecoder::QrTotpWidget widget;

    auto* uriEdit = widget.findChild<QLineEdit*>();
    QVERIFY(uriEdit);

    QCOMPARE(uriEdit->text(), QString());
    QCOMPARE(uriEdit->placeholderText(), QStringLiteral("otpauth://totp/..."));
}

void TestQrTotpWidget::testApplyValidUri()
{
    QrDecoder::QrTotpWidget widget;

    auto* uriEdit = widget.findChild<QLineEdit*>();
    QVERIFY(uriEdit);

    auto* applyButton = findButton(&widget, QStringLiteral("Apply"));
    QVERIFY(applyButton);

    QSignalSpy spy(&widget, &QrDecoder::QrTotpWidget::settingsReady);

    uriEdit->setText(validUri);

    QTest::mouseClick(applyButton, Qt::LeftButton);

    QCOMPARE(spy.count(), 1);

    const auto settings = spy.at(0).at(0).value<QSharedPointer<Totp::Settings>>();
    QVERIFY(settings);

    QCOMPARE(settings->key, QStringLiteral("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
    QCOMPARE(settings->digits, 6);
    QCOMPARE(settings->step, 30);
}

void TestQrTotpWidget::testApplyInvalidUri()
{
    QrDecoder::QrTotpWidget widget;

    auto* uriEdit = widget.findChild<QLineEdit*>();
    QVERIFY(uriEdit);

    auto* applyButton = findButton(&widget, QStringLiteral("Apply"));
    QVERIFY(applyButton);

    QSignalSpy spy(&widget, &QrDecoder::QrTotpWidget::settingsReady);

    uriEdit->setText(QStringLiteral("not-a-totp-uri"));

    QTest::mouseClick(applyButton, Qt::LeftButton);

    QCOMPARE(spy.count(), 0);
}

void TestQrTotpWidget::testPasteValidUri()
{
    QrDecoder::QrTotpWidget widget;

    QSignalSpy spy(&widget, &QrDecoder::QrTotpWidget::settingsReady);

    QApplication::clipboard()->setText(validUri);

    auto* uriEdit = widget.findChild<QLineEdit*>();
    QVERIFY(uriEdit);

    // Trigger the same paste path used by Ctrl+V.
    QTest::keyClick(uriEdit, Qt::Key_V, Qt::ControlModifier);

    QCOMPARE(spy.count(), 1);

    QCOMPARE(uriEdit->text(), validUri);

    const auto settings = spy.at(0).at(0).value<QSharedPointer<Totp::Settings>>();
    QVERIFY(settings);

    QCOMPARE(settings->key, QStringLiteral("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
    QCOMPARE(settings->digits, 6);
    QCOMPARE(settings->step, 30);
}

void TestQrTotpWidget::testPasteInvalidText()
{
    QrDecoder::QrTotpWidget widget;

    QSignalSpy spy(&widget, &QrDecoder::QrTotpWidget::settingsReady);

    QApplication::clipboard()->setText(QStringLiteral("hello world"));

    auto* uriEdit = widget.findChild<QLineEdit*>();
    QVERIFY(uriEdit);

    QTest::keyClick(uriEdit, Qt::Key_V, Qt::ControlModifier);

    QCOMPARE(spy.count(), 0);
    QCOMPARE(uriEdit->text(), QString());
}

void TestQrTotpWidget::testKeyboardPasteValidUri()
{
    QrDecoder::QrTotpWidget widget;

    QSignalSpy spy(&widget, &QrDecoder::QrTotpWidget::settingsReady);

    QApplication::clipboard()->setText(QStringLiteral("  ") + validUri + QStringLiteral("  "));

    auto* uriEdit = widget.findChild<QLineEdit*>();
    QVERIFY(uriEdit);

    QTest::keyClick(uriEdit, Qt::Key_V, Qt::ControlModifier);

    QCOMPARE(spy.count(), 1);

    // pasteClipboard() trims the clipboard text before parsing/storing it.
    QCOMPARE(uriEdit->text(), validUri);

    const auto settings = spy.at(0).at(0).value<QSharedPointer<Totp::Settings>>();
    QVERIFY(settings);

    QCOMPARE(settings->key, QStringLiteral("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
    QCOMPARE(settings->digits, 6);
    QCOMPARE(settings->step, 30);
}
