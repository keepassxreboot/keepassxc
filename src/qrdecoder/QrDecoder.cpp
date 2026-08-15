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

#include "QrDecoder.h"

#include <ZXing/BarcodeFormat.h>
#include <ZXing/ImageView.h>
#include <ZXing/ReadBarcode.h>
#include <ZXing/ReaderOptions.h>

namespace QrDecoder
{
QString QrDecoder::decode(const QImage& input)
{
    if (input.isNull()) {
        return {};
    }

    const auto image = input.convertToFormat(QImage::Format_Grayscale8);

    const auto imageView = ZXing::ImageView(
        image.constBits(),
        image.width(),
        image.height(),
        ZXing::ImageFormat::Lum,
        static_cast<int>(image.bytesPerLine())
    );

    ZXing::ReaderOptions options;
    options.setFormats(
        ZXing::BarcodeFormats{ZXing::BarcodeFormat::QRCode}
    );

    const auto barcodes = ZXing::ReadBarcodes(imageView, options);

    if (barcodes.empty()) {
        return {};
    }

    return QString::fromStdString(barcodes.front().text());
}
} // namespace QrDecoder
