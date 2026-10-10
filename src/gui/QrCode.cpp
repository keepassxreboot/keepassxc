/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "QrCode.h"

#include <QPainter>
#include <QString>
#include <QSvgGenerator>

#include <ZXing/BitMatrix.h>
#include <ZXing/MultiFormatWriter.h>

#include <exception>

struct QrCodePrivate
{
    ZXing::BitMatrix matrix;
};

QrCode::QrCode()
    : d_ptr(new QrCodePrivate())
{
}

QrCode::QrCode(const QString& data, const ErrorCorrectionLevel ecl)
    : d_ptr(new QrCodePrivate())
{
    if (data.isEmpty()) {
        return;
    }

    try {
        // MultiFormatWriter takes the ECC level in the range [0-8] and maps it to QR levels L/M/Q/H
        // as (level - 1) / 2. The quiet zone is drawn by writeSvg(), so request a bare matrix.
        d_ptr->matrix = ZXing::MultiFormatWriter(ZXing::BarcodeFormat::QRCode)
                            .setEccLevel(static_cast<int>(ecl) * 2 + 1)
                            .setMargin(0)
                            .encode(data.toStdString(), 0, 0);
    } catch (const std::exception&) {
        // Data does not fit into a QR code, leave the matrix empty
    }
}

QrCode::~QrCode() = default;

bool QrCode::isValid() const
{
    return !d_ptr->matrix.empty();
}

void QrCode::writeSvg(QIODevice* outputDevice, const int dpi, const int margin) const
{
    if (margin < 0 || !isValid() || outputDevice == nullptr) {
        return;
    }

    const int rowSize = d_ptr->matrix.width();
    const int width = rowSize + margin * 2;

    QSvgGenerator generator;
    generator.setSize(QSize(width, width));
    generator.setViewBox(QRect(0, 0, width, width));
    generator.setResolution(dpi);
    generator.setOutputDevice(outputDevice);

    QPainter painter;
    painter.begin(&generator);

    // Background
    painter.setClipRect(QRect(0, 0, width, width));
    painter.fillRect(QRect(0, 0, width, width), Qt::white);

    // Foreground
    const QPen pen(Qt::black, 0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    const QBrush brush(Qt::black);
    painter.setPen(pen);
    painter.setBrush(brush);

    for (int y = 0; y < rowSize; ++y) {
        for (int x = 0; x < rowSize; ++x) {
            if (d_ptr->matrix.get(x, y)) {
                painter.drawRect(margin + x, margin + y, 1, 1);
            }
        }
    }

    painter.end();
}
