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

#include "TestTotpSetupDialog.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDropEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QSvgRenderer>
#include <QTest>

#include "core/Entry.h"
#include "core/Totp.h"
#include "gui/QrCode.h"
#include "gui/TotpSetupDialog.h"
#include "gui/widgets/QrDropArea.h"

QTEST_MAIN(TestTotpSetupDialog)

namespace
{
    const QString secret = QStringLiteral("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ");

    const QString defaultUri = QStringLiteral("otpauth://totp/ACME%20Co:john@example.com"
                                              "?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"
                                              "&issuer=ACME%20Co&algorithm=SHA1&digits=6&period=30");

    const QString customUri = QStringLiteral("otpauth://totp/ACME%20Co:john@example.com"
                                             "?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"
                                             "&issuer=ACME%20Co&algorithm=SHA256&digits=8&period=60");

    QImage renderQrCode(const QString& text)
    {
        QrCode qrCode(text);
        if (!qrCode.isValid()) {
            return {};
        }

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        qrCode.writeSvg(&buffer, 96);

        QSvgRenderer renderer(buffer.data());
        QImage image(400, 400, QImage::Format_RGB32);
        image.fill(Qt::white);

        QPainter painter(&image);
        renderer.render(&painter);
        return image;
    }

    void dropMimeData(QWidget* target, QMimeData* mimeData)
    {
        // QApplication::notify() only delivers drop events during a real drag session,
        // so dispatch the event to the widget directly
        QDropEvent event(QPointF(10, 10), Qt::CopyAction, mimeData, Qt::LeftButton, Qt::NoModifier);
        static_cast<QObject*>(target)->event(&event);
    }
} // namespace

void TestTotpSetupDialog::init()
{
    m_clipboardText = QApplication::clipboard()->text();
}

void TestTotpSetupDialog::cleanup()
{
    QApplication::clipboard()->setText(m_clipboardText);
}

void TestTotpSetupDialog::testInitialState()
{
    Entry entry;
    TotpSetupDialog dialog(nullptr, &entry);

    auto* otpauthEdit = dialog.findChild<QLineEdit*>("otpauthEdit");
    QVERIFY(otpauthEdit);
    QVERIFY(otpauthEdit->text().isEmpty());

    QVERIFY(dialog.findChild<QrDropArea*>("qrDropArea"));
    QVERIFY(dialog.findChild<QPushButton*>("openImageButton"));
    QVERIFY(dialog.findChild<QPushButton*>("pasteImageButton"));
    QVERIFY(dialog.findChild<QLabel*>("invalidKeyLabel")->isHidden());
    QVERIFY(dialog.findChild<QRadioButton*>("radioDefault")->isChecked());
}

void TestTotpSetupDialog::testApplyDefaultUri()
{
    Entry entry;
    TotpSetupDialog dialog(nullptr, &entry);

    dialog.findChild<QLineEdit*>("otpauthEdit")->setText(defaultUri);

    QCOMPARE(dialog.findChild<QLineEdit*>("seedEdit")->text(), secret);
    QVERIFY(dialog.findChild<QRadioButton*>("radioDefault")->isChecked());
    QVERIFY(dialog.findChild<QLabel*>("invalidKeyLabel")->isHidden());
}

void TestTotpSetupDialog::testApplyCustomUri()
{
    Entry entry;
    TotpSetupDialog dialog(nullptr, &entry);

    dialog.findChild<QLineEdit*>("otpauthEdit")->setText(customUri);

    QCOMPARE(dialog.findChild<QLineEdit*>("seedEdit")->text(), secret);
    QVERIFY(dialog.findChild<QRadioButton*>("radioCustom")->isChecked());
    QCOMPARE(dialog.findChild<QSpinBox*>("stepSpinBox")->value(), 60);
    QCOMPARE(dialog.findChild<QSpinBox*>("digitsSpinBox")->value(), 8);

    auto* algorithmComboBox = dialog.findChild<QComboBox*>("algorithmComboBox");
    QCOMPARE(algorithmComboBox->currentIndex(), algorithmComboBox->findData(Totp::Sha256));
}

void TestTotpSetupDialog::testApplyInvalidUri()
{
    Entry entry;
    TotpSetupDialog dialog(nullptr, &entry);

    auto* otpauthEdit = dialog.findChild<QLineEdit*>("otpauthEdit");
    otpauthEdit->setText(QStringLiteral("otpauth://totp/test?issuer=nosecret"));

    QVERIFY(dialog.findChild<QLineEdit*>("seedEdit")->text().isEmpty());
    QVERIFY(!dialog.findChild<QLabel*>("invalidKeyLabel")->isHidden());

    // Clearing the field hides the error again
    otpauthEdit->clear();
    QVERIFY(dialog.findChild<QLabel*>("invalidKeyLabel")->isHidden());
}

void TestTotpSetupDialog::testDropQrImage()
{
    Entry entry;
    TotpSetupDialog dialog(nullptr, &entry);

    const auto image = renderQrCode(customUri);
    QVERIFY(!image.isNull());

    QMimeData mimeData;
    mimeData.setImageData(image);
    dropMimeData(dialog.findChild<QrDropArea*>("qrDropArea"), &mimeData);

    QCOMPARE(dialog.findChild<QLineEdit*>("otpauthEdit")->text(), customUri);
    QCOMPARE(dialog.findChild<QLineEdit*>("seedEdit")->text(), secret);
    QCOMPARE(dialog.findChild<QSpinBox*>("digitsSpinBox")->value(), 8);
    QVERIFY(dialog.findChild<QLabel*>("invalidKeyLabel")->isHidden());
}

void TestTotpSetupDialog::testDropImageWithoutQrCode()
{
    Entry entry;
    TotpSetupDialog dialog(nullptr, &entry);

    QImage image(200, 200, QImage::Format_RGB32);
    image.fill(Qt::white);

    QMimeData mimeData;
    mimeData.setImageData(image);
    dropMimeData(dialog.findChild<QrDropArea*>("qrDropArea"), &mimeData);

    QVERIFY(dialog.findChild<QLineEdit*>("otpauthEdit")->text().isEmpty());
    QVERIFY(dialog.findChild<QLineEdit*>("seedEdit")->text().isEmpty());
    QVERIFY(!dialog.findChild<QLabel*>("invalidKeyLabel")->isHidden());
}

void TestTotpSetupDialog::testPasteQrImage()
{
    Entry entry;
    TotpSetupDialog dialog(nullptr, &entry);

    const auto image = renderQrCode(defaultUri);
    QVERIFY(!image.isNull());
    QApplication::clipboard()->setImage(image);

    QTest::mouseClick(dialog.findChild<QPushButton*>("pasteImageButton"), Qt::LeftButton);

    QCOMPARE(dialog.findChild<QLineEdit*>("otpauthEdit")->text(), defaultUri);
    QCOMPARE(dialog.findChild<QLineEdit*>("seedEdit")->text(), secret);
}

void TestTotpSetupDialog::testPasteUri()
{
    Entry entry;
    TotpSetupDialog dialog(nullptr, &entry);

    QApplication::clipboard()->setText(QStringLiteral("  ") + customUri + QStringLiteral("  "));

    QTest::mouseClick(dialog.findChild<QPushButton*>("pasteImageButton"), Qt::LeftButton);

    QCOMPARE(dialog.findChild<QLineEdit*>("otpauthEdit")->text(), customUri);
    QCOMPARE(dialog.findChild<QLineEdit*>("seedEdit")->text(), secret);
}

void TestTotpSetupDialog::testKeyboardPasteImageIntoUriField()
{
    Entry entry;
    TotpSetupDialog dialog(nullptr, &entry);

    const auto image = renderQrCode(customUri);
    QVERIFY(!image.isNull());
    QApplication::clipboard()->setImage(image);

    auto* otpauthEdit = dialog.findChild<QLineEdit*>("otpauthEdit");
    QTest::keyClick(otpauthEdit, Qt::Key_V, Qt::ControlModifier);

    QCOMPARE(otpauthEdit->text(), customUri);
    QCOMPARE(dialog.findChild<QLineEdit*>("seedEdit")->text(), secret);
}

void TestTotpSetupDialog::testKeyboardPasteTextIntoUriField()
{
    Entry entry;
    TotpSetupDialog dialog(nullptr, &entry);

    QApplication::clipboard()->setText(defaultUri);

    auto* otpauthEdit = dialog.findChild<QLineEdit*>("otpauthEdit");
    QTest::keyClick(otpauthEdit, Qt::Key_V, Qt::ControlModifier);

    // Text is pasted by the line edit as usual
    QCOMPARE(otpauthEdit->text(), defaultUri);
    QCOMPARE(dialog.findChild<QLineEdit*>("seedEdit")->text(), secret);
}

void TestTotpSetupDialog::testSaveSettings()
{
    Entry entry;
    // The dialog deletes itself when closed
    QPointer<TotpSetupDialog> dialog = new TotpSetupDialog(nullptr, &entry);
    QSignalSpy spy(dialog.data(), &TotpSetupDialog::totpUpdated);

    dialog->findChild<QLineEdit*>("otpauthEdit")->setText(customUri);
    dialog->findChild<QDialogButtonBox*>("buttonBox")->button(QDialogButtonBox::Ok)->click();

    QCOMPARE(spy.count(), 1);

    const auto settings = entry.totpSettings();
    QVERIFY(settings);
    QCOMPARE(settings->key, secret);
    QCOMPARE(settings->digits, 8u);
    QCOMPARE(settings->step, 60u);
    QCOMPARE(settings->algorithm, Totp::Sha256);

    delete dialog.data();
}
