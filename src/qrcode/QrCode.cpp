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
#include "QrCode_p.h"

#include <QByteArray>
#include <QPainter>
#include <QString>
#include <QSvgGenerator>

QrCodePrivate::QrCodePrivate()
    : m_qrcode(nullptr)
{
}

QrCodePrivate::~QrCodePrivate()
{
    if (m_qrcode) {
        QRcode_free(m_qrcode);
    }
}

QrCode::QrCode()
    : d_ptr(new QrCodePrivate())
{
}

QrCode::QrCode(const QString& data, const Version version, const ErrorCorrectionLevel ecl, const bool caseSensitive)
    : d_ptr(new QrCodePrivate())
{
    init(data, version, ecl, caseSensitive);
}

QrCode::QrCode(const QByteArray& data, const Version version, const ErrorCorrectionLevel ecl)
    : d_ptr(new QrCodePrivate())
{
    init(data, version, ecl);
}

QrCode::~QrCode() = default;

void QrCode::init(const QString& data, const Version version, const ErrorCorrectionLevel ecl, bool caseSensitive)
{
    if (data.isEmpty()) {
        return;
    }

    d_ptr->m_qrcode = QRcode_encodeString(data.toLocal8Bit().data(),
                                          static_cast<int>(version),
                                          static_cast<QRecLevel>(ecl),
                                          QR_MODE_8,
                                          caseSensitive ? 1 : 0);
}

void QrCode::init(const QByteArray& data, const Version version, const ErrorCorrectionLevel ecl)
{
    if (data.isEmpty()) {
        return;
    }

    d_ptr->m_qrcode = QRcode_encodeData(data.size(),
                                        reinterpret_cast<const unsigned char*>(data.data()),
                                        static_cast<int>(version),
                                        static_cast<QRecLevel>(ecl));
}

bool QrCode::isValid() const
{
    return d_ptr->m_qrcode != nullptr;
}

void QrCode::writeSvg(QIODevice* outputDevice, const int dpi, const int margin) const
{
    if (margin < 0 || d_ptr->m_qrcode == nullptr || outputDevice == nullptr) {
        return;
    }

    const int width = d_ptr->m_qrcode->width + margin * 2;

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
    // "Dots" are stored in a quint8 x quint8 array using row-major order.
    // A dot is black if the LSB of its corresponding quint8 is 1.
    const QPen pen(Qt::black, 0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    const QBrush brush(Qt::black);
    painter.setPen(pen);
    painter.setBrush(brush);

    const int rowSize = d_ptr->m_qrcode->width;
    unsigned char* dot = d_ptr->m_qrcode->data;
    for (int y = 0; y < rowSize; ++y) {
        for (int x = 0; x < rowSize; ++x) {
            if (quint8(0x01) == (static_cast<quint8>(*dot++) & quint8(0x01))) {
                painter.drawRect(margin + x, margin + y, 1, 1);
            }
        }
    }

    painter.end();
}
