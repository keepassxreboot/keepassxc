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

#include "QRCode.h"
#include "QRCode_p.h"

#include <QImage>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QString>
#include <QByteArray>

QRCodePrivate::QRCodePrivate() : m_qrcode(nullptr)
{
}

QRCodePrivate::~QRCodePrivate()
{
    if (nullptr != m_qrcode)
       QRcode_free(m_qrcode);
}

QRCode::QRCode() : d_ptr(new QRCodePrivate())
{
}

QRCode::QRCode(const QString& data,
    const Version version,
    const ErrorCorrectionLevel ecl,
    const bool caseSensitive)
    : d_ptr(new QRCodePrivate())
{
    init(data, version, ecl, caseSensitive);
}

QRCode::QRCode(const QByteArray& data,
    const Version version,
    const ErrorCorrectionLevel ecl)
    : d_ptr(new QRCodePrivate())
{
    init(data, version, ecl);
}

QRCode::~QRCode() =default;

void QRCode::init(const QString& data,
    const Version version,
    const ErrorCorrectionLevel ecl,
    bool caseSensitive)
{
    if (data.isEmpty())
        return;

    d_ptr->m_qrcode = QRcode_encodeString(
        data.toLocal8Bit().data(),
        static_cast<int>(version),
        static_cast<QRecLevel>(ecl),
        QR_MODE_8,
        caseSensitive ? 1 : 0);
}

void QRCode::init(const QByteArray& data,
  const Version version,
  const ErrorCorrectionLevel ecl)
{
    if (data.isEmpty())
        return;

    d_ptr->m_qrcode = QRcode_encodeData(
        data.size(),
        reinterpret_cast<const unsigned char*>(data.data()),
        static_cast<int>(version),
        static_cast<QRecLevel>(ecl));
}

QImage QRCode::toQImage(const int size, const int margin) const
{
    if (size <= 0 || margin < 0 || nullptr == d_ptr->m_qrcode)
        return QImage();

    const int width = d_ptr->m_qrcode->width + margin * 2;
    QImage img(QSize(width,width), QImage::Format_Mono);

    QPainter painter;
    painter.begin(&img);

    // Background
    painter.setClipRect(QRect(0, 0, width, width));
    painter.fillRect(QRect(0, 0, width, width), Qt::white);

    // Foreground
    // "Dots" are stored in a quint8 x quint8 array using row-major order.
    // A dot is black if the LSB of its corresponding quint8 is 1.
    const QPen pen(Qt::black, 0.1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    const QBrush brush(Qt::black);
    painter.setPen(pen);
    painter.setBrush(brush);

    const int rowSize = d_ptr->m_qrcode->width;
    unsigned char* dot = d_ptr->m_qrcode->data;
    for (int y = 0; y < rowSize; ++y) {
        for (int x = 0; x < rowSize; ++x) {
            if (quint8(0x01) == (static_cast<quint8>(*dot++) & quint8(0x01)))
                painter.drawRect(margin + x, margin + y, 1, 1);
      }
    }

    painter.end();

    return img.scaled(size, size);
}
