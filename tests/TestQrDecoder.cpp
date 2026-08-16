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

#include "TestQrDecoder.h"

#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QTest>

#include "qrcode/QrCode.h"
#include "qrdecoder/QrDecoder.h"

QTEST_GUILESS_MAIN(TestQrDecoder)

void TestQrDecoder::testNullImage()
{
    const QImage image;

    QVERIFY(image.isNull());
    QVERIFY(QrDecoder::decode(image).isEmpty());
}

void TestQrDecoder::testEmptyImage()
{
    QImage image(200, 200, QImage::Format_RGB32);
    image.fill(Qt::white);

    QVERIFY(QrDecoder::decode(image).isEmpty());
}

void TestQrDecoder::testDecodeTotpQrCode()
{
    const QString secret =
        "otpauth://totp/"
        "ACME%20Co:john@example.com?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ&issuer=ACME%20Co&algorithm="
        "SHA1&digits=6&period=30";

    QrCode qrCode(secret, QrCode::Version::AUTO, QrCode::ErrorCorrectionLevel::MEDIUM);

    QVERIFY(qrCode.isValid());

    QByteArray svgData;
    QBuffer buffer(&svgData);

    QVERIFY(buffer.open(QIODevice::WriteOnly));

    qrCode.writeSvg(&buffer, 96);
    buffer.close();

    QVERIFY(!svgData.isEmpty());

    QSvgRenderer renderer(svgData);
    QVERIFY(renderer.isValid());

    constexpr int imageSize = 512;

    QImage image(imageSize, imageSize, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    QVERIFY(painter.isActive());

    renderer.render(&painter);
    painter.end();

    QCOMPARE(QrDecoder::decode(image), secret);
}
